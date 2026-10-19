#include "qte.h"
#include "QtCore/qassert.h"

/*!
* \brief Worker thread for simulation.
* \param context The shared OpenGL context.
* \param sim The simulation.
* \param useGPU Use GPU for simulation.
* \param debug Debug mode.
* \param parent The parent object.
*/
Worker::Worker(QOpenGLContext* context, QOffscreenSurface* surface, EcosystemSimulation* sim, bool debug, QObject* parent)
    :  QObject(parent), context(context), surface(surface), debug(debug), running(false), paused(false), init(false), simulator(sim)
{
    nbSteps = -1;
}

/*!
* \brief Destructor.
*/
Worker::~Worker()
{
}

/*!
* \brief Initialize the OpenGL context.
*/
void Worker::initContext()
{
    assert(QThread::currentThread() == context->thread());

    // Make the context current in this thread
    context->makeCurrent(surface);

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        qWarning() << "GLEW error in worker thread: " << glewGetErrorString(err);
        return;
    }

    context->doneCurrent();

    init = true;
}

/*!
* \brief Play the simulation.
*/
void Worker::play() {
    // Init context if not already done
    if (!init) initContext();
    if (!simulator) return;

    // Init the simulation
    qDebug() << "Simulation started/resumed.";
    running = true;
    paused = false;
    step = 0;

    // Main loop
    while (running) {
        // Pause the simulation
        if (paused && nbSteps ==-1) {
            QThread::msleep(100);  // Delay to avoid 100% CPU usage
            continue;
        }

        // Number of steps, if specific steps are provided pause the simulation
        int n = 1;
        if (nbSteps != -1) {
            paused = true;
            n = nbSteps;
        }

        // Simulate
        context->makeCurrent(surface);
        simulator->SimulateYear(n, debug);

        // Inside the context: anything that reads a texture back needs it current, and a
        // readback without one silently returns zeros instead of failing
        size_t nbTrees = paused ? simulator->GetNbTrees() : 0;

        context->doneCurrent();

        step+=n;
        emit stepCompleted(step);
        if (paused) {
            qDebug() << "Number of trees: " << nbTrees;
            emit batchCompleted(); // Update resource maps in the editor
        }
        nbSteps = -1;
    }
    emit finished();
    qDebug() << "Simulation finished.";
}

/*!
* \brief Set the number of steps to simulate.
* \param n The number of steps.
*/
void Worker::steps(int n)
{
    nbSteps = n;
}

/*!
* \brief Pause the simulation.
*/
void Worker::pause() {
    paused = true;
    qDebug() << "Simulation paused.";
}

/*!
* \brief Resume the simulation.
*/
void Worker::resume() {
    paused = false;
    qDebug() << "Simulation resumed.";
}

/*!
* \brief Stop the simulation.
*/
void Worker::stop() {
    running = false;
    paused = false;
    qDebug() << "Simulation stopped.";
}

/*!
* \brief Reload the simulation.
*/
void Worker::reload() {
    simulator->Clear();
    step = 0;
    emit stepCompleted(step);
}
