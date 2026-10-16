// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/engine/dag.h>

#include <piejam/algorithm/transform_to_vector.h>
#include <piejam/audio/engine/dag_executor.h>
#include <piejam/audio/engine/event_buffer_memory.h>
#include <piejam/audio/engine/rt_task_executor.h>
#include <piejam/audio/engine/thread_context.h>
#include <piejam/range/indices.h>
#include <piejam/thread/cache_line_size.h>
#include <piejam/thread/cpu_clock.h>
#include <piejam/thread/cpu_util.h>

#include <boost/assert.hpp>
#include <boost/lockfree/stack.hpp>

#include <algorithm>
#include <atomic>
#include <numeric>
#include <ranges>
#include <span>
#include <vector>

namespace piejam::audio::engine
{

namespace
{

class dag_executor_base : public dag_executor
{
protected:
    dag_executor_base(dag::tasks_t const& tasks, dag::graph_t const& graph)
        : m_nodes(make_nodes(tasks, graph))
    {
    }

    struct node;
    using node_ref = std::reference_wrapper<node>;

    struct node
    {
        std::atomic_size_t parents_to_process{};
        std::size_t num_parents{};
        dag::task_t task;
        std::vector<node_ref> children;
    };

    static void init_node_for_process(node& n)
    {
        n.parents_to_process.store(n.num_parents, std::memory_order_relaxed);
    }

    using nodes_t = std::unordered_map<dag::task_id_t, node>;

    nodes_t m_nodes;

private:
    static auto make_nodes(dag::tasks_t const& tasks, dag::graph_t const& graph)
        -> nodes_t
    {
        nodes_t nodes;
        for (auto const& [id, task] : tasks)
        {
            nodes[id].task = task;
        }

        for (auto const& [parent_id, children] : graph)
        {
            BOOST_ASSERT(nodes.contains(parent_id));
            std::ranges::transform(
                children,
                std::back_inserter(nodes[parent_id].children),
                [&nodes](dag::task_id_t const child_id) {
                    BOOST_ASSERT(nodes.contains(child_id));
                    return std::ref(nodes[child_id]);
                });

            for (dag::task_id_t const child_id : children)
            {
                ++nodes[child_id].num_parents;
            }
        }

        return nodes;
    }
};

class dag_executor_st final : public dag_executor_base
{
public:
    dag_executor_st(
        dag::tasks_t const& tasks,
        dag::graph_t const& graph,
        std::size_t const event_memory_size)
        : dag_executor_base(tasks, graph)
        , m_event_memory(event_memory_size)
    {
        m_run_queue.reserve(m_nodes.size());

        // do a dry run to gather a static execution order

        std::vector<node*> run_queue;
        run_queue.reserve(m_nodes.size());

        for (auto& [id, nd] : m_nodes)
        {
            init_node_for_process(nd);

            if (nd.num_parents == 0)
            {
                BOOST_ASSERT(run_queue.size() < run_queue.capacity());
                run_queue.emplace_back(std::addressof(nd));
            }
        }

        while (!run_queue.empty())
        {
            node* const nd = run_queue.back();
            run_queue.pop_back();

            BOOST_ASSERT(
                nd->parents_to_process.load(std::memory_order_relaxed) == 0);

            // store into real run queue
            m_run_queue.emplace_back(nd);

            for (node& child : nd->children)
            {
                if (1 == child.parents_to_process.fetch_sub(
                             1,
                             std::memory_order_relaxed))
                {
                    BOOST_ASSERT(run_queue.size() < run_queue.capacity());
                    run_queue.push_back(std::addressof(child));
                }
            }
        }
    }

    auto operator()(std::size_t const buffer_size)
        -> std::chrono::nanoseconds override
    {
        auto const start = thread::cpu_clock::now();

        m_thread_context.buffer_size = buffer_size;

        for (node* const nd : m_run_queue)
        {
            nd->task(m_thread_context);
        }

        m_event_memory.release();

        return thread::cpu_clock::now() - start;
    }

private:
    audio::engine::event_buffer_memory m_event_memory;
    audio::engine::thread_context m_thread_context{
        .event_memory = &m_event_memory.memory_resource()};
    std::vector<node*> m_run_queue;
};

class dag_executor_mt final : public dag_executor_base
{
public:
    static constexpr std::size_t job_queue_capacity = 1024;
    using jobs_t = boost::lockfree::stack<
        node*,
        boost::lockfree::fixed_sized<true>,
        boost::lockfree::capacity<job_queue_capacity>>;

    dag_executor_mt(
        dag::tasks_t const& tasks,
        dag::graph_t const& graph,
        std::size_t const event_memory_size,
        std::span<rt_task_executor> const worker_threads)
        : dag_executor_base(tasks, graph)
        , m_worker_threads(worker_threads)
        , m_initial_tasks(collect_initial_tasks(m_nodes))
        , m_main_worker(
              event_memory_size,
              m_running_counter,
              m_nodes_to_process,
              m_buffer_size,
              m_run_queue)
        , m_workers(make_workers(
              worker_threads.size(),
              event_memory_size,
              m_running_counter,
              m_nodes_to_process,
              m_buffer_size,
              m_run_queue))
    {
    }

    auto operator()(std::size_t const buffer_size)
        -> std::chrono::nanoseconds override
    {
        m_buffer_size.store(buffer_size, std::memory_order_relaxed);

        for (auto&& [id, n] : m_nodes)
        {
            init_node_for_process(n);
        }

        for (node* const n : m_initial_tasks)
        {
            m_run_queue.unsynchronized_push(n);
        }

        m_nodes_to_process.store(m_nodes.size(), std::memory_order_relaxed);

        BOOST_ASSERT(m_workers.size() == m_worker_threads.size());
        for (std::size_t const w : range::indices(m_worker_threads))
        {
            // Wrap into a reference_wrapper here to guarantee small-object
            // optimization inside the worker thread.
            m_worker_threads[w].wakeup(std::ref(m_workers[w]));
        }

        m_main_worker();

        BOOST_ASSERT(m_nodes_to_process.load(std::memory_order_relaxed) == 0);

        // busy wait until all workers finished
        while (m_running_counter.load(std::memory_order_acquire) > 0)
        {
            std::atomic_signal_fence(std::memory_order_seq_cst);

            this_thread::cpu_spin_yield();
        }

        return std::chrono::nanoseconds{
            std::accumulate(
                m_workers.begin(),
                m_workers.end(),
                m_main_worker.cpu_load(),
                [](auto const acc, auto const& w) {
                    return acc + w.cpu_load();
                })
                .count() /
            m_num_all_workers};
    }

private:
    static auto collect_initial_tasks(nodes_t& nodes) -> std::vector<node*>
    {
        std::vector<node*> initial_tasks;
        initial_tasks.reserve(nodes.size());

        for (auto&& [id, node] : nodes)
        {
            if (node.num_parents == 0)
            {
                initial_tasks.push_back(std::addressof(node));
            }
        }

        return initial_tasks;
    }

    struct dag_worker
    {
        dag_worker(
            std::size_t const event_memory_size,
            std::atomic_size_t& running_counter,
            std::atomic_size_t& nodes_to_process,
            std::atomic_size_t& buffer_size,
            jobs_t& run_queue)
            : m_event_memory(event_memory_size)
            , m_running(running_counter)
            , m_nodes_to_process(nodes_to_process)
            , m_buffer_size(buffer_size)
            , m_run_queue(run_queue)
        {
        }

        [[nodiscard]]
        auto cpu_load() const noexcept -> std::chrono::nanoseconds
        {
            return m_cpu_load;
        }

        void operator()()
        {
            m_running.fetch_add(1, std::memory_order_release);

            auto const cpu_load_start = thread::cpu_clock::now();

            m_thread_context.buffer_size =
                m_buffer_size.load(std::memory_order_relaxed);

            while (m_nodes_to_process.load(std::memory_order_acquire))
            {
                node* n{};
                if (m_run_queue.pop(n))
                {
                    while (n)
                    {
                        n = process_node(*n);
                    }
                }
            }

            m_event_memory.release();

            m_cpu_load = thread::cpu_clock::now() - cpu_load_start;

            BOOST_VERIFY(0 < m_running.fetch_sub(1, std::memory_order_release));
        }

    private:
        auto process_node(node& n) -> node*
        {
            BOOST_ASSERT(
                n.parents_to_process.load(std::memory_order_relaxed) == 0);

            n.task(m_thread_context);

            node* next{};
            for (node& child : n.children)
            {
                if (1 == child.parents_to_process.fetch_sub(
                             1,
                             std::memory_order_acq_rel))
                {
                    if (next)
                    {
                        m_run_queue.push(std::addressof(child));
                    }
                    else
                    {
                        next = std::addressof(child);
                    }
                }
            }

            BOOST_VERIFY(
                0 < m_nodes_to_process.fetch_sub(1, std::memory_order_acq_rel));

            return next;
        }

        std::chrono::nanoseconds m_cpu_load{};
        audio::engine::event_buffer_memory m_event_memory;
        audio::engine::thread_context m_thread_context{
            .event_memory = &m_event_memory.memory_resource()};
        std::atomic_size_t& m_running;
        std::atomic_size_t& m_nodes_to_process;
        std::atomic_size_t& m_buffer_size;
        jobs_t& m_run_queue;

        static_assert(std::atomic_size_t::is_always_lock_free);
    };

    using workers_t = std::vector<dag_worker>;

    static auto make_workers(
        std::size_t const num_workers,
        std::size_t const event_memory_size,
        std::atomic_size_t& running_counter,
        std::atomic_size_t& nodes_to_process,
        std::atomic_size_t& buffer_size,
        jobs_t& run_queue) -> workers_t
    {
        workers_t workers;
        workers.reserve(num_workers);

        for (std::size_t i = 1; i < num_workers + 1; ++i)
        {
            workers.emplace_back(
                event_memory_size,
                running_counter,
                nodes_to_process,
                buffer_size,
                run_queue);
        }

        return workers;
    }

    alignas(thread::cache_line_size) std::atomic_size_t m_running_counter{};
    alignas(thread::cache_line_size) std::atomic_size_t m_nodes_to_process{};
    std::span<rt_task_executor> m_worker_threads;
    std::vector<node*> const m_initial_tasks;
    jobs_t m_run_queue;
    std::atomic_size_t m_buffer_size{};
    dag_worker m_main_worker;
    workers_t m_workers;
    std::size_t m_num_all_workers{1 + m_workers.size()};
};

auto
is_descendent(
    dag::graph_t const& t,
    dag::task_id_t const parent,
    dag::task_id_t const descendent) -> bool
{
    if (parent == descendent)
    {
        return true;
    }

    return std::ranges::any_of(
        t.at(parent),
        [&t, descendent](dag::task_id_t const child) {
            return is_descendent(t, child, descendent);
        });
}

} // namespace

dag::dag() = default;

auto
dag::add_task(task_t t) -> task_id_t
{
    auto id = m_free_id++;
    m_tasks.emplace_back(id, std::move(t));
    m_graph[id];
    return id;
}

auto
dag::add_child_task(task_id_t const parent, task_t t) -> task_id_t
{
    auto it = m_graph.find(parent);

    BOOST_ASSERT_MSG(it != m_graph.end(), "parent node not found");

    auto id = add_task(std::move(t));
    it->second.push_back(id);
    return id;
}

void
dag::add_child(task_id_t const parent, task_id_t const child)
{
    auto it_parent = m_graph.find(parent);

    BOOST_ASSERT_MSG(it_parent != m_graph.end(), "parent node not found");

    auto it_child = m_graph.find(child);

    BOOST_ASSERT_MSG(it_child != m_graph.end(), "child node not found");

    BOOST_ASSERT_MSG(
        !is_descendent(m_graph, it_child->first, it_parent->first),
        "child is ancestor of the parent");

    it_parent->second.push_back(it_child->first);
}

auto
dag::make_runnable(
    std::span<rt_task_executor> const worker_threads,
    std::size_t const event_memory_size) -> std::unique_ptr<dag_executor>
{
    if (worker_threads.empty())
    {
        return std::make_unique<dag_executor_st>(
            m_tasks,
            m_graph,
            event_memory_size);
    }

    return std::make_unique<dag_executor_mt>(
        m_tasks,
        m_graph,
        event_memory_size,
        worker_threads);
}

} // namespace piejam::audio::engine
