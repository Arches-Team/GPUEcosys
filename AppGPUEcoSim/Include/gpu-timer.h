#pragma once

#include "gpu-shader.h"

#include <array>
#include <cstdint>
#include <vector>

/*!
* \brief GPU-side timing of the simulation compute passes, using GL_TIME_ELAPSED queries.
*
* The QElapsedTimer measurements in EcosystemSimulation::SimulateYear are CPU wall-clock;
* they only approximate GPU cost because GPUSync() blocks after every dispatch. This class
* measures the GPU directly, so that the synchronization can be relaxed without losing the
* ability to measure.
*
* Results are harvested with kFrames years of latency, so reading them back does not stall
* the CPU. Flush() collects whatever is still in flight at the end of a run.
*
* \note GL_TIME_ELAPSED queries cannot nest: Begin() / End() pairs must be sequential.
* \note Every method requires the OpenGL context to be current (i.e. the worker thread).
*/
class GPUTimer
{
public:
    enum Pass
    {
        Canopy = 0,
        Stamp,
        LIF,
        RegenLayers,
        Individuals,
        Seeding,
        SeedSource,
        SeedProb,
        PassCount
    };

    //! Nanoseconds spent in each pass over one year.
    using PassTimes = std::array<uint64_t, PassCount>;

    void Reset(); //!< Drop the frames in flight and clear the history
    void Destroy(); //!< Delete the query objects

    void BeginFrame(); //!< Start a year, harvesting the oldest frame still in flight
    void Begin(Pass p);
    void End();
    void Flush(); //!< Harvest every frame still in flight, oldest first (may block)

    //! One entry per harvested year, in simulation order.
    const std::vector<PassTimes>& History() const { return history; }

private:
    struct Slot
    {
        GLuint query = 0;
        Pass pass = PassCount;
    };

    // NB: do not name a member "slots" -- Qt #defines it away (qobjectdefs.h)
    struct Frame
    {
        std::vector<Slot> queries;
        int used = 0;
        bool pending = false;
    };

    void Harvest(Frame& f);

    // Enough lag that a harvested frame is always already complete.
    static constexpr int kFrames = 3;

    std::array<Frame, kFrames> frames;
    std::vector<PassTimes> history;
    int current = -1;
    bool inQuery = false;
};

inline void GPUTimer::Reset()
{
    for (Frame& f : frames)
    {
        f.used = 0;
        f.pending = false;
    }
    history.clear();
    current = -1;
    inQuery = false;
}

inline void GPUTimer::Destroy()
{
    for (Frame& f : frames)
    {
        for (Slot& s : f.queries)
            if (s.query != 0)
                glDeleteQueries(1, &s.query);
        f.queries.clear();
        f.used = 0;
        f.pending = false;
    }
    history.clear();
    current = -1;
    inQuery = false;
}

inline void GPUTimer::BeginFrame()
{
    current = (current + 1) % kFrames;

    Frame& f = frames[current];
    if (f.pending) // kFrames years old, so the results are ready and this does not stall
        Harvest(f);

    f.used = 0;
    f.pending = true;
}

inline void GPUTimer::Begin(Pass p)
{
    if (current < 0 || inQuery) // BeginFrame() was not called, or the query would nest
        return;

    Frame& f = frames[current];
    if (f.used == int(f.queries.size()))
    {
        Slot s;
        glGenQueries(1, &s.query);
        f.queries.push_back(s);
    }

    Slot& s = f.queries[f.used];
    s.pass = p;
    glBeginQuery(GL_TIME_ELAPSED, s.query);
    inQuery = true;
}

inline void GPUTimer::End()
{
    if (!inQuery)
        return;

    glEndQuery(GL_TIME_ELAPSED);
    frames[current].used++;
    inQuery = false;
}

inline void GPUTimer::Flush()
{
    // Oldest first, so that the history stays in simulation order
    for (int i = 1; i <= kFrames; ++i)
    {
        Frame& f = frames[(current + i) % kFrames];
        if (f.pending)
            Harvest(f);
    }
}

inline void GPUTimer::Harvest(Frame& f)
{
    PassTimes t{};
    for (int i = 0; i < f.used; ++i)
    {
        // A pass dispatched several times a year (StepIndividuals) accumulates
        GLuint64 ns = 0;
        glGetQueryObjectui64v(f.queries[i].query, GL_QUERY_RESULT, &ns);
        if (f.queries[i].pass < PassCount)
            t[f.queries[i].pass] += ns;
    }

    f.pending = false;
    history.push_back(t);
}
