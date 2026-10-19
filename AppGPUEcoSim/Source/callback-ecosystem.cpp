#include "qte.h"
#include <QtWidgets/qcolordialog.h>
#include <QtWidgets/QFileDialog>

void QteWindow::createActionsEcosystem()
{
    // Preset 
    connect(uiw.eco_preset_pyrenean, SIGNAL(clicked()), this, SLOT(CallInitEcoPyrenean()));
    connect(uiw.eco_preset_berchtesgaden, SIGNAL(clicked()), this, SLOT(CallInitEcoBerchtesgaden()));

    // Species selection
    connect(uiw.eco_species_list, SIGNAL(currentRowChanged(int)), this, SLOT(CallEcoSpeciesSelected(int)));

    // Species parameters
    connect(uiw.eco_species_name, SIGNAL(editingFinished()), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_a_max, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_a, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_ct, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_kg, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_gr, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_hmax, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_k1, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_k2, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_k3, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_lgs, SIGNAL(stateChanged(int)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_pa_max, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_r_m, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_r_s, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_r_sl, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_r_t, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_c_m, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_c_s, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_c_sl, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_c_t, SIGNAL(valueChanged(double)), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_enabled, SIGNAL(clicked()), this, SLOT(CallEcoSpeciesEdited()));
    connect(uiw.eco_species_color, SIGNAL(clicked()), this, SLOT(CallEcoSpeciesSelectColor()));

    // Edit maps
    connect(uiw.pushButton_Eco_Edit_Seed, SIGNAL(clicked()), this, SLOT(CallEcoEditSeeds()));
    connect(uiw.pushButton_Eco_Edit_Saplings, SIGNAL(clicked()), this, SLOT(CallEcoEditSaplings()));
    connect(uiw.pushButton_Eco_Edit_Mature, SIGNAL(clicked()), this, SLOT(CallEcoEditMature()));
    connect(uiw.pushButton_Eco_Edit_Boost, SIGNAL(clicked()), this, SLOT(CallEcoEditBoost()));
    connect(uiw.pushButton_Eco_View_LIF, SIGNAL(clicked()), this, SLOT(CallEcoViewLIF()));

    connect(uiw.pushButton_Eco_Regen_Seed, SIGNAL(clicked()), this, SLOT(CallEcoRegenSeeds()));
    connect(uiw.pushButton_Eco_Clear_Seed, SIGNAL(clicked()), this, SLOT(CallEcoClearSeeds()));
    connect(uiw.pushButton_Eco_Clear_Saplings, SIGNAL(clicked()), this, SLOT(CallEcoClearSaplings()));
    connect(uiw.pushButton_Eco_Clear_Mature, SIGNAL(clicked()), this, SLOT(CallEcoClearMature()));
    connect(uiw.pushButton_Eco_Clear_Boost, SIGNAL(clicked()), this, SLOT(CallEcoClearBoost()));


    connect(mayawidgetEditor, SIGNAL(changedSpecies(MapSpecies)), this, SLOT(CallMapChanged(MapSpecies)));

    // Load - Save
    connect(uiw.pushButton_Eco_Save_Params, SIGNAL(clicked()), this, SLOT(CallSaveEcosystemParams()));
    connect(uiw.pushButton_Eco_Load_Params, SIGNAL(clicked()), this, SLOT(CallLoadEcosystemParams()));


    // init ecosystem
    CallInitEcoPyrenean();
    uiw.eco_species_list->setCurrentRow(0);
}


void QteWindow::LoadEcosystem(const QString& fileName)
{
    ecosystemFile = fileName;

    uiw.eco_species_list->blockSignals(true);
    uiw.eco_species_list->clear();
    uiw.eco_species_list->blockSignals(false);

    ecosystemData.clear();

    ecosystemData.LoadSpeciesParam(fileName);
    ecosystemData.LoadLIPs(QDir(SOLUTION_DIR "data/Lips"));

    // ====== Species Params ======
    QVector<QString> speciesNames = ecosystemData.getListSpecies();

    // Fill the list
    for (int i = 0; i < speciesNames.size(); ++i)
    {
        uiw.eco_species_list->addItem(speciesNames[i]);
    }
    uiw.groupBox_eco_TreeParameters->setEnabled(false);
    uiw.groupBox_eco_TreeResponse->setEnabled(false);


    uiw.eco_species_list->blockSignals(true);
    uiw.eco_species_list->setCurrentRow(0);
    uiw.eco_species_list->blockSignals(false);

    uiw.dbg_species->setMaximum(speciesNames.size() - 1);
}

void QteWindow::CallInitEcoPyrenean()
{
    QString fileName = SOLUTION_DIR "data/Presets/fixSun_v18.json";
    LoadEcosystem(fileName);
    if (sim)
        sim->LoadParams();
}

void QteWindow::CallInitEcoBerchtesgaden()
{
    QString fileName = SOLUTION_DIR "data/Presets/Berchtesgaden_v9.json";
    LoadEcosystem(fileName);

    if (sim)
        sim->LoadParams();
}

/*!
* /brief Call when a species is selected
* /param species Species index
*/
void QteWindow::CallEcoSpeciesSelected(int species)
{
    qDebug() << "CallEcoSpeciesSelected()";

    SpeciesParam param = ecosystemData.GetSpeciesParam(species);

    uiw.eco_species_name->blockSignals(true);
    uiw.eco_species_name->setText(param.name);
    uiw.eco_species_name->blockSignals(false);

    uiw.eco_species_a_max->blockSignals(true);
    uiw.eco_species_a_max->setValue(param.a_max);
    uiw.eco_species_a_max->blockSignals(false);

    uiw.eco_species_hmax->blockSignals(true);
    uiw.eco_species_hmax->setValue(param.hmax);
    uiw.eco_species_hmax->blockSignals(false);

    uiw.eco_species_a->blockSignals(true);
    uiw.eco_species_a->setValue(param.a);
    uiw.eco_species_a->blockSignals(false);

    uiw.eco_species_ct->blockSignals(true);
    uiw.eco_species_ct->setValue(param.ct);
    uiw.eco_species_ct->blockSignals(false);

    uiw.eco_species_kg->blockSignals(true);
    uiw.eco_species_kg->setValue(param.q);
    uiw.eco_species_kg->blockSignals(false);

    uiw.eco_species_gr->blockSignals(true);
    uiw.eco_species_gr->setValue(param.gr);
    uiw.eco_species_gr->blockSignals(false);

    uiw.eco_species_pa_max->blockSignals(true);
    uiw.eco_species_pa_max->setValue(param.pa_max);
    uiw.eco_species_pa_max->blockSignals(false);

    uiw.eco_species_k1->blockSignals(true);
    uiw.eco_species_k1->setValue(param.k1);
    uiw.eco_species_k1->blockSignals(false);

    uiw.eco_species_k2->blockSignals(true);
    uiw.eco_species_k2->setValue(param.k2);
    uiw.eco_species_k2->blockSignals(false);

    uiw.eco_species_k3->blockSignals(true);
    uiw.eco_species_k3->setValue(param.k3);
    uiw.eco_species_k3->blockSignals(false);

    uiw.eco_species_lgs->blockSignals(true);
    uiw.eco_species_lgs->setChecked(param.longGrowthSeason);
    uiw.eco_species_lgs->blockSignals(false);

    uiw.eco_species_c_s->blockSignals(true);
    uiw.eco_species_c_s->setValue(param.c_s);
    uiw.eco_species_c_s->blockSignals(false);

    uiw.eco_species_r_s->blockSignals(true);
    uiw.eco_species_r_s->setValue(param.r_s);
    uiw.eco_species_r_s->blockSignals(false);

    uiw.eco_species_c_m->blockSignals(true);
    uiw.eco_species_c_m->setValue(param.c_m);
    uiw.eco_species_c_m->blockSignals(false);

    uiw.eco_species_r_m->blockSignals(true);
    uiw.eco_species_r_m->setValue(param.r_m);
    uiw.eco_species_r_m->blockSignals(false);

    uiw.eco_species_c_t->blockSignals(true);
    uiw.eco_species_c_t->setValue(param.c_t);
    uiw.eco_species_c_t->blockSignals(false);

    uiw.eco_species_r_t->blockSignals(true);
    uiw.eco_species_r_t->setValue(param.r_t);
    uiw.eco_species_r_t->blockSignals(false);

    uiw.eco_species_r_sl->blockSignals(true);
    uiw.eco_species_r_sl->setValue(param.r_sl);
    uiw.eco_species_r_sl->blockSignals(false);

    uiw.eco_species_c_sl->blockSignals(true);
    uiw.eco_species_c_sl->setValue(param.c_sl);
    uiw.eco_species_c_sl->blockSignals(false);

    uiw.eco_species_enabled->blockSignals(true);
    uiw.eco_species_enabled->setChecked(param.enabled);
    uiw.eco_species_enabled->blockSignals(false);


    uiw.eco_species_color->setStyleSheet("background-color: " + param.col.GetQt().name());

    uiw.groupBox_eco_TreeParameters->setEnabled(true);
    uiw.groupBox_eco_TreeResponse->setEnabled(true);

    if (sim) updateEcosystemPixmap();
    mayawidgetEditor->setSpecies(species);

    updateEditRange();
}

/*!
* /brief Call when a species is edited
*/
void QteWindow::CallEcoSpeciesEdited()
{
    int species = uiw.eco_species_list->currentRow();
    SpeciesParam param = ecosystemData.GetSpeciesParam(species);

    param.name = uiw.eco_species_name->text();
    param.a_max = uiw.eco_species_a_max->value();
    param.hmax = uiw.eco_species_hmax->value();
    param.a = uiw.eco_species_a->value();
    param.ct = uiw.eco_species_ct->value();
    param.q = uiw.eco_species_kg->value();
    param.gr = uiw.eco_species_gr->value();
    param.pa_max = uiw.eco_species_pa_max->value();
    param.k1 = uiw.eco_species_k1->value();
    param.k2 = uiw.eco_species_k2->value();
    param.k3 = uiw.eco_species_k3->value();
    param.longGrowthSeason = uiw.eco_species_lgs->isChecked();
    param.enabled = uiw.eco_species_enabled->isChecked();

    param.c_s = uiw.eco_species_c_s->value();
    param.r_s = uiw.eco_species_r_s->value();
    param.c_m = uiw.eco_species_c_m->value();
    param.r_m = uiw.eco_species_r_m->value();
    param.c_t = uiw.eco_species_c_t->value();
    param.r_t = uiw.eco_species_r_t->value();
    param.r_sl = uiw.eco_species_r_sl->value();
    param.c_sl = uiw.eco_species_c_sl->value();

    ecosystemData.SetSpeciesParam(species, param);

    if (sim)
        sim->LoadParams();

    updateEditRange();
}

/*!
* /brief Call when a species color is selected
*/
void QteWindow::CallEcoSpeciesSelectColor()
{
    qDebug() << "CallEcoSpeciesSelectColor()" ;

    int species = uiw.eco_species_list->currentRow();
    SpeciesParam param = ecosystemData.GetSpeciesParam(species);

    QColor color = QColorDialog::getColor(param.col.GetQt());
    if (color.isValid())
    {
        uiw.eco_species_color->setStyleSheet("background-color: " + color.name());
        param.col = Color(color);

        ecosystemData.SetSpeciesParam(species, param);

        if (sim)
            sim->LoadParams();
    }
}


void QteWindow::CallEcoSpeciesUpdateData()
{
    uiw.groupBox_eco_Seeding->setEnabled(mapEditionEnabled);
    uiw.groupBox_eco_Saplings->setEnabled(mapEditionEnabled);
    uiw.groupBox_eco_Individuals->setEnabled(mapEditionEnabled);
    uiw.groupBox_eco_Boost->setEnabled(mapEditionEnabled);
    uiw.groupBox_eco_LIF->setEnabled(mapEditionEnabled);

    updateEcosystemPixmap();
}

void QteWindow::updateEcosystemPixmap()
{
    int species = uiw.eco_species_list->currentRow();

    if (species < 0)
        return;

    if (sim) sim->fillEcosystemData();

    mayawidgetEditor->setSpecies(species);

    uiw.eco_species_Seed->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::SEED_PROBS));
    uiw.eco_species_Saplings->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::REGEN_LAYERS));
    uiw.eco_species_Individuals->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::TREES));
    uiw.eco_species_Boost->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::BOOST));

    uiw.eco_species_LIF->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::LIF));

}

void QteWindow::CallMapChanged(const MapSpecies& type)
{
    qDebug() << "Map changed ";
    int species = uiw.eco_species_list->currentRow();

    if (species < 0)
        return;

    ecosystemData.SyncImages();

    switch (type)
    {
    case MapSpecies::SEED_PROBS:
        uiw.eco_species_Seed->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::SEED_PROBS));
        break;
    case MapSpecies::REGEN_LAYERS:
        uiw.eco_species_Saplings->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::REGEN_LAYERS));
        break;
    case MapSpecies::TREES:
        uiw.eco_species_Individuals->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::TREES));
        break;
    case MapSpecies::BOOST:
        uiw.eco_species_Boost->setPixmap(ecosystemData.GetPixmap(species, MapSpecies::BOOST));
        break;
    }

    if (sim) sim->LoadEcosystem(worker->isPaused());
}


void QteWindow::CallEcoEditSeeds()
{
    mayawidgetEditor->editSelectedMap(MapSpecies::SEED_PROBS, uiw.eco_species_list->currentRow());
    updateEditRange();
}
void QteWindow::CallEcoEditSaplings()
{
    mayawidgetEditor->editSelectedMap(MapSpecies::REGEN_LAYERS, uiw.eco_species_list->currentRow());
    updateEditRange();
}
void QteWindow::CallEcoEditMature()
{
    mayawidgetEditor->editSelectedMap(MapSpecies::TREES, uiw.eco_species_list->currentRow());
    updateEditRange();
}
void QteWindow::CallEcoEditBoost()
{
    mayawidgetEditor->editSelectedMap(MapSpecies::BOOST, uiw.eco_species_list->currentRow());
    updateEditRange();
}
void QteWindow::CallEcoViewLIF()
{
    mayawidgetEditor->editSelectedMap(MapSpecies::LIF, -1);
}

void QteWindow::CallEcoRegenSeeds()
{
    sim->ComputeSeedProb();

    CallEcoSpeciesUpdateData();
}

void QteWindow::CallEcoClearSeeds()
{
    ScalarField2& sf = ecosystemData.GetScalarField(MapSpecies::SEED_PROBS, uiw.eco_species_list->currentRow());
    sf.Fill(0.0);

    CallMapChanged(MapSpecies::SEED_PROBS);
}

void QteWindow::CallEcoClearSaplings()
{
    ScalarField2& sf = ecosystemData.GetScalarField(MapSpecies::REGEN_LAYERS, uiw.eco_species_list->currentRow());
    sf.Fill(0.0);

    CallMapChanged(MapSpecies::REGEN_LAYERS);
}

void QteWindow::CallEcoClearMature()
{
    ScalarField2& sf = ecosystemData.GetScalarField(MapSpecies::TREES, uiw.eco_species_list->currentRow());
    sf.Fill(0.0);

    CallMapChanged(MapSpecies::TREES);
}

void QteWindow::CallEcoClearBoost()
{
    ScalarField2& sf = ecosystemData.GetScalarField(MapSpecies::BOOST, uiw.eco_species_list->currentRow());
    sf.Fill(0.0);

    CallMapChanged(MapSpecies::BOOST);
}

void QteWindow::CallSaveEcosystemParams()
{
    ecosystemData.SaveSpeciesParam();
}

void QteWindow::CallLoadEcosystemParams()
{
    QString filename = QFileDialog::getOpenFileName(nullptr, "Load Species Parameters", SOLUTION_DIR "data/Presets/", "JSON files (*.json)");
    if (filename.isEmpty())
        return;
    LoadEcosystem(filename);

    if (sim)
        sim->LoadParams();
}
