
#include "qte.h"

void QteWindow::createActionsEditor()
{
    connect(uiw.doubleSpinBox_EditorTool_radius, SIGNAL(valueChanged(double)), mayawidgetEditor, SLOT(setRadius(double)));
    connect(uiw.horizontalSlider_EditorTool_intensity, SIGNAL(valueChanged(int)), mayawidgetEditor, SLOT(setIntensity(int)));
    connect(uiw.doubleSpinBox_EditorTool_intensity, SIGNAL(valueChanged(double)), mayawidgetEditor, SLOT(setIntensity(double)));

    connect(uiw.horizontalSlider_EditorTool_intensity, SIGNAL(valueChanged(int)), this, SLOT(setValueIntensity(int)));
    connect(uiw.doubleSpinBox_EditorTool_intensity, SIGNAL(valueChanged(double)), this, SLOT(setValueIntensity(double)));
    connect(uiw.selection_X, SIGNAL(valueChanged(int)), this, SLOT(setSelection()));
    connect(uiw.selection_Y, SIGNAL(valueChanged(int)), this, SLOT(setSelection()));

    connect(mayawidgetEditor, SIGNAL(valueRadiusChanged(double)), uiw.doubleSpinBox_EditorTool_radius, SLOT(setValue(double)));
    connect(mayawidgetEditor, SIGNAL(valueIntensityChanged(double)), uiw.doubleSpinBox_EditorTool_intensity, SLOT(setValue(double)));
    connect(mayawidgetEditor, SIGNAL(valueIntensityChanged(double)), this, SLOT(setValueIntensity(double)));
    connect(mayawidgetEditor, SIGNAL(selectionChanged(const Vector&)), this, SLOT(CallSelectionChanged(const Vector&)));

    connect(uiw.doubleSpinBox_nbTrees, SIGNAL(valueChanged(double)), mayawidgetEditor, SLOT(setSamples(double)));

    // Put buttons in a list to manage easily
    QList<QPushButton*> buttons = { uiw.pushButton_Eco_EditGaussian, uiw.pushButton_Eco_EditSamples };

    // Lambda that handles exclusive toggle logic
    auto makeExclusive = [=](QPushButton* clickedButton) {
        for (QPushButton* btn : buttons) {
            btn->setChecked(btn == clickedButton); // Only clicked button stays checked
        }
    };

    // Connect all buttons to the same logic
    for (QPushButton* btn : buttons) {
        QObject::connect(btn, &QPushButton::clicked, [=]() { makeExclusive(btn); });
        connect(btn, SIGNAL(clicked()), this, SLOT(CallUpdateTool()));
    }

}


/*!
* \brief Enable or disable the edition of the maps, and the corresponding buttons.
* \param enabled True to allow editing (simulation paused/stopped), false while it runs.
*/
void QteWindow::SetMapEditionEnabled(bool enabled)
{
    mapEditionEnabled = enabled;
    if (mayawidgetEditor)
        mayawidgetEditor->setEditEnabled(enabled);

    uiw.pushButton_Env_Edit_SunExposure->setEnabled(enabled);
    uiw.pushButton_Env_Edit_Moisture->setEnabled(enabled);
    uiw.pushButton_Env_Edit_Temperature->setEnabled(enabled);
    uiw.pushButton_Env_Edit_Mask->setEnabled(enabled);

    uiw.pushButton_Env_Gen_SunExposure->setEnabled(enabled);
    uiw.pushButton_Env_Gen_Moisture->setEnabled(enabled);
    uiw.pushButton_Env_Gen_Temperature->setEnabled(enabled);
    uiw.pushButton_Env_Gen_Mask->setEnabled(enabled);

    uiw.pushButton_Env_Load_SunExposure->setEnabled(enabled);
    uiw.pushButton_Env_Load_Moisture->setEnabled(enabled);
    uiw.pushButton_Env_Load_Temperature->setEnabled(enabled);
    uiw.pushButton_Env_Save_Mask->setEnabled(enabled);
    uiw.load_env->setEnabled(enabled);

    // Ecosystem boxes stay disabled until data is loaded: they are re-enabled by CallEcoSpeciesUpdateData()
    if (!enabled)
    {
        uiw.groupBox_eco_Seeding->setEnabled(false);
        uiw.groupBox_eco_Saplings->setEnabled(false);
        uiw.groupBox_eco_Individuals->setEnabled(false);
        uiw.groupBox_eco_Boost->setEnabled(false);
        uiw.groupBox_eco_LIF->setEnabled(false);
    }

    uiw.groupBox_ToolsParameters->setEnabled(enabled);
}

void QteWindow::CallSelectionChanged(const Vector& pos)
{
    Vector2 offset = resourceData.GetHeightField().GetArray().Size() / 2;
    offset += Vector2(pos);

    uiw.selection_X->blockSignals(true);
    uiw.selection_Y->blockSignals(true);
    uiw.selection_X->setValue(offset[0]);
    uiw.selection_Y->setValue(offset[1]);
    uiw.selection_X->blockSignals(false);
    uiw.selection_Y->blockSignals(false);

    if (sim)
        sim->Observe(pos);
}

void QteWindow::setSelection()
{
    Vector2 size = resourceData.GetHeightField().GetArray().Size();
    int x = uiw.selection_X->value();
    int y = uiw.selection_Y->value();
    if (x >= size[0]) {
        x = size[0] - 1;
        uiw.selection_X->setValue(x);
    }
    if (y >= size[1]) {
        y = size[1] - 1;
        uiw.selection_Y->setValue(y);
    }

    Vector2 offset = size / 2;
    Vector selection (x - offset[0], y - offset[1], resourceData.GetHeightField().Value(x / 2, y / 2));
    mayawidgetEditor->select(selection);

    if (sim)
        sim->Observe(Vector2(x - offset[0], y - offset[1]));
}

void QteWindow::CallUpdateTool()
{
    if (mayawidgetEditor == nullptr) return;
    if (uiw.pushButton_Eco_EditGaussian->isChecked())
    {
        mayawidgetEditor->setTool(TOOL_GAUSSIAN);
        uiw.doubleSpinBox_nbTrees->setEnabled(false);
    }
    else if (uiw.pushButton_Eco_EditSamples->isChecked())
    {
        mayawidgetEditor->setTool(TOOL_RANDOM);
        uiw.doubleSpinBox_nbTrees->setEnabled(true);
    }
    else
    {
        mayawidgetEditor->setTool(TOOL_NONE);
    }
}
