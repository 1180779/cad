/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#include "CncPanelWidget.hxx"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <utility>

#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>

#include "OpenGLWidget.hpp"
#include <cad_common/util/MultiSignalBlocker.hxx>
#include <cad_common/widgets/WidgetBuilders.hxx>

#include "cnc/Simulator.hxx"

CncPanelWidget::CncPanelWidget(OpenGlWidget* glWidget, QWidget* parent)
    : ToolPanelWidget("CNC", parent),
      m_gl(glWidget) {
    buildUi();
    connectSignals();
    refresh();

    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setTimerType(Qt::CoarseTimer);
    m_refreshTimer.setInterval(OpenGlWidget::gc_cncPollInterval);
    connect(&m_refreshTimer, &QTimer::timeout, this, &CncPanelWidget::refresh);
}

void CncPanelWidget::showEvent(QShowEvent* event) {
    ToolPanelWidget::showEvent(event);
    if (!m_hasShown) {
        m_hasShown = true;
        if (m_gl) {
            m_gl->setCncVisible(true);
            m_gl->focusCncView();
        }
    }
    refresh();
}

void CncPanelWidget::hideEvent(QHideEvent* event) {
    ToolPanelWidget::hideEvent(event);
    m_refreshTimer.stop();
}

void CncPanelWidget::buildUi() {
    // ReSharper disable CppDFAMemoryLeak
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignTop);

    const auto addGroup = [content, layout](const QString& title) {
        auto* group = new QGroupBox(title, content);
        layout->addWidget(group);
        auto* groupLayout = new QVBoxLayout(group);
        groupLayout->setContentsMargins(8, 8, 8, 8);
        return std::pair{group, groupLayout};
    };

    {
        const auto [group, groupLayout] = addGroup("Toolpath");
        const widgets::LayoutBuilder controls(groupLayout, {.parent = group});
        m_loadButton = controls.addButton("Load toolpath files…");
        m_clearButton = controls.addButton("Clear programs");
        m_files = controls.addList(90);
    }

    {
        const auto [group, groupLayout] = addGroup("Material");
        auto* form = new QFormLayout;
        form->setContentsMargins(0, 0, 0, 0);
        const widgets::FormBuilder controls(form,
        {
            .parent = group,
            .spinBox = {
                .minimum = cnc::Material::gc_minSubdivisions,
                .maximum = cnc::Material::gc_maxSubdivisions,
                .value = cnc::Material::gc_defaultSubdivisions
            },
            .doubleSpinBox = {
                .minimum = -1000.0,
                .maximum = 1000.0,
                .value = 0.0
            },
        });
        m_width = controls.addDoubleSpinBox("Width [mm]", {
            .minimum = 1.0,
            .value = 100.0
        });
        m_depth = controls.addDoubleSpinBox("Depth [mm]", {
            .minimum = 1.0,
            .value = 100.0
        });
        m_thickness = controls.addDoubleSpinBox("Thickness [mm]", {
            .minimum = 1.0,
            .maximum = 200.0,
            .value = 20.0
        });
        m_centerX = controls.addDoubleSpinBox("Centre X [mm]");
        m_centerY = controls.addDoubleSpinBox("Centre Y [mm]");
        m_topZ = controls.addDoubleSpinBox("Top Z [mm]");
        m_subdivisionsX = controls.addSpinBox("Subdivisions X");
        m_subdivisionsY = controls.addSpinBox("Subdivisions Y");
        groupLayout->addLayout(form);

        m_applyMaterialButton = new QPushButton("Apply material", group);
        m_resetMaterialButton = new QPushButton("Reset material", group);
        groupLayout->addWidget(m_applyMaterialButton);
        groupLayout->addWidget(m_resetMaterialButton);
    }

    {
        const auto [group, groupLayout] = addGroup("Simulation");
        const widgets::LayoutBuilder controls(groupLayout, {.parent = group});
        auto* buttons = new QWidget(group);
        auto* buttonLayout = new QHBoxLayout(buttons);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        const widgets::LayoutBuilder buttonControls(buttonLayout, {.parent = buttons});
        m_playButton = buttonControls.addButton("Play");
        m_finishButton = buttonControls.addButton("Finish now");
        groupLayout->addWidget(buttons);

        static_cast<void>(controls.addLabel("Speed"));
        m_speed = controls.addSlider(Qt::Horizontal, 0, 1000, 303);
        m_speedLabel = controls.addLabel();

        m_progress = controls.addProgressBar(0, 1000, false, 10);

        m_statusLabel = controls.addLabel("Ready", true);
        m_timeLabel = controls.addLabel("0.00 / 0.00 s");
        m_moveLabel = controls.addLabel("Instructions: 0");
    }

    {
        const auto [group, groupLayout] = addGroup("Display");
        const widgets::LayoutBuilder controls(groupLayout, {.parent = group});
        m_showBlock = controls.addCheckBox("Show block in scene", true);
        m_showPaths = controls.addCheckBox("Show toolpaths", true);
        m_showTool = controls.addCheckBox("Show tool", true);
        m_flatShading = controls.addCheckBox("Flat shading", false);
        m_fitButton = controls.addButton("Fit view to stock");
    }

    {
        const auto [group, groupLayout] = addGroup("Warnings");
        m_warnings = widgets::LayoutBuilder(groupLayout, {.parent = group}).addList(150);
    }

    layout->addStretch(1);
    createScrollLayout(content);
    // ReSharper restore CppDFAMemoryLeak
}

void CncPanelWidget::connectSignals() {
    using CNC = CncPanelWidget;
    connect(m_loadButton, &QPushButton::clicked, this, &CNC::loadPathFiles);
    connect(m_clearButton, &QPushButton::clicked, this, &CNC::clearPrograms);
    connect(m_applyMaterialButton, &QPushButton::clicked, this, &CNC::applyMaterial);
    connect(m_resetMaterialButton, &QPushButton::clicked, this, &CNC::resetMaterial);
    connect(m_playButton, &QPushButton::clicked, this, &CNC::togglePlayback);
    connect(m_finishButton, &QPushButton::clicked, this, &CNC::finishSimulation);
    connect(m_fitButton, &QPushButton::clicked, this, &CNC::fitViewToStock);
    connect(m_speed, &QSlider::valueChanged, this, &CNC::updateSpeed);
    connect(m_showBlock, &QCheckBox::toggled, this, &CNC::setBlockVisible);
    connect(m_showPaths, &QCheckBox::toggled, this, &CNC::setPathsVisible);
    connect(m_showTool, &QCheckBox::toggled, this, &CNC::setToolVisible);
    connect(m_flatShading, &QCheckBox::toggled, this, &CNC::setFlatShading);
    connect(m_gl, &OpenGlWidget::cncChanged, this, &CNC::startTimer);

    m_speedLabel->setText(QString::number(speedFromSlider(m_speed->value()), 'f', 2) + "x");
    m_gl->setCncSpeed(speedFromSlider(m_speed->value()));
}

void CncPanelWidget::finishSimulation() const {
    m_gl->finishCncNow();
}

void CncPanelWidget::fitViewToStock() const {
    m_gl->focusCncView();
}

void CncPanelWidget::updateSpeed(const int value) const {
    const double speed = speedFromSlider(value);
    m_speedLabel->setText(QString::number(speed, 'f', 2) + "x");
    m_gl->setCncSpeed(speed);
}

void CncPanelWidget::setBlockVisible(const bool checked) const {
    m_gl->setCncVisible(checked);
    if (checked) {
        m_gl->focusCncView();
    }
}

void CncPanelWidget::setPathsVisible(const bool checked) const {
    m_gl->setCncShowPaths(checked);
}

void CncPanelWidget::setToolVisible(const bool checked) const {
    m_gl->setCncShowTool(checked);
}

void CncPanelWidget::setFlatShading(const bool checked) const {
    m_gl->setCncFlatShading(checked);
}

void CncPanelWidget::startTimer() {
    if (isVisible() && !m_refreshTimer.isActive()) {
        m_refreshTimer.start();
    }
}


double CncPanelWidget::speedFromSlider(const int value) {
    const double t = static_cast<double>(value) / 1000.0;
    return 0.1 * std::pow(2000.0, t);
}

cnc::Material CncPanelWidget::materialFromControls() const {
    return cnc::Material{
        .width = static_cast<float>(m_width->value()),
        .depth = static_cast<float>(m_depth->value()),
        .thickness = static_cast<float>(m_thickness->value()),
        .centerX = static_cast<float>(m_centerX->value()),
        .centerY = static_cast<float>(m_centerY->value()),
        .topZ = static_cast<float>(m_topZ->value()),
        .subdivisionsX = m_subdivisionsX->value(),
        .subdivisionsY = m_subdivisionsY->value(),
    }.normalized();
}

void CncPanelWidget::setMaterialControls(const cnc::Material& material) const {
    const MultiSignalBlocker blockers{
        m_width, m_depth, m_thickness, m_centerX, m_centerY, m_topZ,
        m_subdivisionsX, m_subdivisionsY,
    };
    m_width->setValue(material.width);
    m_depth->setValue(material.depth);
    m_thickness->setValue(material.thickness);
    m_centerX->setValue(material.centerX);
    m_centerY->setValue(material.centerY);
    m_topZ->setValue(material.topZ);
    m_subdivisionsX->setValue(material.subdivisionsX);
    m_subdivisionsY->setValue(material.subdivisionsY);
}

void CncPanelWidget::applyMaterial() {
    const cnc::Material material = materialFromControls();
    m_gl->setCncMaterial(material);
    m_lastWarningCount = static_cast<std::size_t>(-1);
    m_lastSuppressedWarnings = static_cast<std::size_t>(-1);
}

void CncPanelWidget::resetMaterial() {
    m_gl->resetCncMaterial();
    m_progress->setValue(0);
    m_statusLabel->setText("Material reset");
    m_lastWarningCount = static_cast<std::size_t>(-1);
    m_lastSuppressedWarnings = static_cast<std::size_t>(-1);
}

void CncPanelWidget::clearPrograms() {
    m_gl->clearCncPrograms();
    m_files->clear();
    m_warnings->clear();
    m_progress->setValue(0);
    m_statusLabel->setText("No programs loaded");
    m_moveLabel->setText("Instructions: 0");
    m_lastWarningCount = static_cast<std::size_t>(-1);
    m_lastSuppressedWarnings = static_cast<std::size_t>(-1);
}

void CncPanelWidget::togglePlayback() const {
    const auto snapshot = m_gl->cncSnapshot();
    if (snapshot && snapshot->playing) {
        m_gl->setCncPlaying(false);
        return;
    }
    if (snapshot && snapshot->finished) {
        m_gl->resetCncMaterial();
    }
    m_gl->setCncPlaying(true);
}

void CncPanelWidget::loadPathFiles() {
    const QStringList paths = QFileDialog::getOpenFileNames(
        this,
        "Load toolpath files",
        {},
        "NC files (*.k* *.f* *.nc *.tap);;All files (*)"
        );
    for (const QString& path : paths) {
        loadPathFile(path);
    }
}

void CncPanelWidget::loadPathFile(const QString& path) {
    const auto result = cnc::parseProgramFile(
        std::filesystem::path(path.toStdString()),
        std::nullopt
        );

    const bool hasErrors = std::ranges::any_of(result.diagnostics,
        [](const cnc::Diagnostic& diagnostic) {
            return diagnostic.type == cnc::DiagnosticMessageType::Error;
        }
        );
    if (!result.program || hasErrors) {
        QString message = "Could not load:\n" + path + "\n";
        for (const cnc::Diagnostic& diagnostic : result.diagnostics) {
            message += QString("\nline %1: %2")
                       .arg(diagnostic.line)
                       .arg(QString::fromStdString(diagnostic.message));
        }
        QMessageBox::warning(this, "Toolpath load failed", message);
        return;
    }

    const cnc::Program& program = result.program.value();
    const bool firstProgram = m_files->count() == 0;

    const QString displayName = QString::fromStdString(program.name);
    const auto [type, diameter] = program.tool;
    m_gl->appendCncProgram(result.program.value());
    m_files->addItem(
        QString("%1  [%2 %3 mm]")
        .arg(displayName)
        .arg(QString::fromStdString(std::string(cnc::toolTypeName(type))))
        .arg(diameter)
        );

    if (firstProgram) {
        m_gl->focusCncView();
    }
    m_statusLabel->setText("Loaded " + displayName);
}

void CncPanelWidget::refresh() {
    if (!m_gl) {
        return;
    }

    const auto snapshot = m_gl->cncSnapshot();
    if (!snapshot) {
        return;
    }

    m_playButton->setText(snapshot->playing ? "Pause" : "Play");
    m_progress->setValue(
        static_cast<int>(std::clamp(snapshot->progress, 0.0f, 1.0f) * 1000.0f)
        );
    m_timeLabel->setText(
        QString("%1 / %2 s")
        .arg(snapshot->elapsed, 0, 'f', 2)
        .arg(snapshot->totalDuration, 0, 'f', 2)
        );
    m_moveLabel->setText(
        QString("Instructions: %1 / %2   GPU queue: %3")
        .arg(snapshot->instructionCounter)
        .arg(snapshot->instructionCount)
        .arg(m_gl->pendingCncCutCount())
        );

    if (!m_gl->cncError().empty()) {
        m_statusLabel->setText(QString::fromStdString(m_gl->cncError()));
    } else if (snapshot->finished) {
        m_statusLabel->setText("Finished");
    } else if (snapshot->playing) {
        m_statusLabel->setText("Milling in progress");
    } else if (snapshot->hasPrograms) {
        m_statusLabel->setText("Paused");
    } else {
        m_statusLabel->setText("No programs loaded");
    }

    if (snapshot->warnings.size() != m_lastWarningCount
        || snapshot->suppressedWarnings != m_lastSuppressedWarnings) {
        updateWarnings();
    }

    setMaterialControls(snapshot->material);

    // Keep polling only while the timeline is advancing or while GPU cutter
    // moves are still being drained. Once both are idle the timer disarms.
    if (!isVisible()) {
        return;
    }
    if (snapshot->playing || m_gl->pendingCncCutCount() > 0) {
        m_refreshTimer.start();
    }
}

void CncPanelWidget::updateWarnings() {
    const auto snapshot = m_gl->cncSnapshot();
    if (!snapshot) {
        return;
    }

    m_lastWarningCount = snapshot->warnings.size();
    m_lastSuppressedWarnings = snapshot->suppressedWarnings;
    m_warnings->clear();

    for (const cnc::Warning& warning : snapshot->warnings) {
        auto* item = new QListWidgetItem(
            QString("%1: %2")
            .arg(QString::fromStdString(std::string(cnc::warningKindName(warning.kind))))
            .arg(QString::fromStdString(warning.message)),
            m_warnings
            );
        item->setToolTip(
            QString("Move %1, program %2")
            .arg(warning.moveIndex)
            .arg(warning.programIndex)
            );
        if (warning.kind == cnc::WarningKind::tooDeep
            || warning.kind == cnc::WarningKind::verticalPlunge) {
            item->setForeground(gc_criticalErrorColor);
        } else {
            item->setForeground(gc_errorColor);
        }
    }
    if (snapshot->suppressedWarnings > 0) {
        auto* item = new QListWidgetItem(
            QString("... %1 more warnings suppressed").arg(snapshot->suppressedWarnings),
            m_warnings
            );
        item->setForeground(gc_warningColor);
    }
}
