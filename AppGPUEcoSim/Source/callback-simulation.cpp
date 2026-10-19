
#include "qte.h"
#include "histogram.h"

/*!
\brief Create callbacks between member slots and user interface.
*/
void QteWindow::createActionsSimulation()
{

    connect(uiw.sim_seed, SIGNAL(clicked()), this, SLOT(CallSeed()));
    connect(uiw.pushButton_SimStart, SIGNAL(clicked()), this, SLOT(CallSimStart()));
    connect(uiw.pushButton_SimPause, SIGNAL(clicked()), this, SLOT(CallSimPause()));
    connect(uiw.pushButton_SimStop, SIGNAL(clicked()), this, SLOT(CallSimStop()));
    connect(uiw.pushButton_SimReload, SIGNAL(clicked()), this, SLOT(CallSimReload()));
    connect(uiw.pushButton_SimStep, SIGNAL(clicked()), this, SLOT(CallSimYear()));
    connect(uiw.pushButton_SimSteps, SIGNAL(clicked()), this, SLOT(CallSimYears()));

}

/*!
*   \brief Initialize the simulation with seeds
*/
void QteWindow::CallSeed()
{
    qDebug() << "CallSeed() " << sim;
    if (sim)
    {
        sim->Seeding();
        CallEcoSpeciesUpdateData();
        Update();
    }
}

/*!
\brief Start the simulation.
*/
void QteWindow::CallSimStart()
{
    SetMapEditionEnabled(false);

    if (!workerThread) {
        qDebug() << "CallSimStart() " << sim;

        // Get the main context
        QOpenGLContext* mainContext = mayawidget->context();

        QOpenGLContext* workerContext = new QOpenGLContext;
        workerContext->setFormat(mainContext->format());
        workerContext->setShareContext(mainContext);
        workerContext->create();

        QOffscreenSurface* surface = new QOffscreenSurface;
        surface->setFormat(workerContext->format());
        surface->create();

        workerThread = new QThread;
        worker = new Worker(workerContext, surface, sim, uiw.stats->isChecked());

        worker->moveToThread(workerThread);
        workerContext->moveToThread(workerThread);
        surface->moveToThread(workerThread);

        connect(workerThread, &QThread::started, worker, &Worker::play);
        connect(worker, &Worker::finished, this, &QteWindow::onSimulationFinished);
        connect(worker, &Worker::finished, workerThread, &QThread::quit);
        connect(workerThread, &QThread::finished, worker, &Worker::deleteLater);
        connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);
        connect(worker, &Worker::stepCompleted, this, &QteWindow::UpdateStep);
        connect(worker, &Worker::batchCompleted, this, &QteWindow::UpdateBatch);

        workerThread->start();
    }
    else {
        worker->resume();
    }
}

/*!
* \brief Pause the simulation.
*/
void QteWindow::CallSimPause()
{
    if (worker && !worker->isPaused())
    {
        worker->pause();

        SetMapEditionEnabled(true);
        CallEcoSpeciesUpdateData();
    }
}

/*!
* \brief Stop the simulation.
*/
void QteWindow::CallSimStop()
{
    CallSimReload();
    if (worker) {
        disconnect(worker, nullptr, this, nullptr);
        worker->stop();
        worker = nullptr;
    }
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();  // Attendre que le thread se termine
        workerThread = nullptr;
        worker = nullptr;
    }
    SetMapEditionEnabled(true);
}

/*!
* \brief Reload the simulation.
*/
void QteWindow::CallSimReload()
{
    CallSimPause();
    if (worker)
    {
        worker->reload();
        CallEcoSpeciesUpdateData();
        sim->Clear();
    }
}

/*!
* \brief Perform a single year simulation.
*/
void QteWindow::CallSimYear()
{
    qDebug() << "CallSimYear()";

    CallSimStart();
    worker->steps(1);
}

/*!
* \brief Perform N year simulations.
*/
void QteWindow::CallSimYears()
{
    qDebug() << "CallSimYears()";

    CallSimStart();
    worker->steps(uiw.nbYears->value());
}

/*!
* \brief Update the simulation when it is finished.
*/
void QteWindow::onSimulationFinished()
{
    qDebug() << "Simulation finished.";
    CallSimStop();
}

/*!
* \brief Update the simulation step.
* \param step The current simulation step.
*/
void QteWindow::UpdateStep(int step)
{
    uiw.nbYearsSimulated->setValue(step);
    Update();
}

/*!
* \brief Update the simulation batch.
*/
void QteWindow::UpdateBatch()
{
    // A batch is only emitted once the worker paused itself after N steps
    SetMapEditionEnabled(true);
    CallEcoSpeciesUpdateData();
}
