/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CAD_SRC_GUI_TOOLBARS_CNC_CNC_PANEL_WIDGET_HXX
#define CAD_HEADER_SRC_CAD_SRC_GUI_TOOLBARS_CNC_CNC_PANEL_WIDGET_HXX

#include <cstddef>

#include <QPointer>
#include <QTimer>

#include <cnc/Material.hxx>

#include "../ToolPanelWidget.hpp"

class OpenGlWidget;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QProgressBar;
class QPushButton;
class QSlider;
class QSpinBox;

class CncPanelWidget final : public ToolPanelWidget
{
public:
    explicit CncPanelWidget(OpenGlWidget* glWidget, QWidget* parent = nullptr);

    /// @brief Load one NC file (used by command-line preview helpers)
    void loadPathFile(const QString& path);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    static constexpr QColor gc_criticalErrorColor{217, 83, 79};
    static constexpr QColor gc_errorColor{217, 83, 79};
    static constexpr QColor gc_warningColor{217, 83, 79};

    void buildUi();
    void connectSignals();
    void refresh();
    void loadPathFiles();
    void applyMaterial();
    void resetMaterial();
    void clearPrograms();
    void togglePlayback() const;
    void finishSimulation() const;
    void fitViewToStock() const;
    void updateSpeed(int value) const;
    void setBlockVisible(bool checked) const;
    void setPathsVisible(bool checked) const;
    void setToolVisible(bool checked) const;
    void setFlatShading(bool checked) const;
    void setStockCollisions(bool checked) const;
    void setMaterialControls(const cnc::Material& material) const;
    void updateWarnings();

    [[nodiscard]] cnc::Material materialFromControls() const;
    [[nodiscard]] static double speedFromSlider(int value);

    QPointer<OpenGlWidget> m_gl{};

    QTimer m_refreshTimer{this};

    QListWidget* m_files{};
    QListWidget* m_warnings{};
    QPushButton* m_loadButton{};
    QPushButton* m_clearButton{};
    QPushButton* m_playButton{};
    QPushButton* m_finishButton{};
    QPushButton* m_applyMaterialButton{};
    QPushButton* m_resetMaterialButton{};
    QPushButton* m_fitButton{};
    QDoubleSpinBox* m_width{};
    QDoubleSpinBox* m_depth{};
    QDoubleSpinBox* m_thickness{};
    QDoubleSpinBox* m_centerX{};
    QDoubleSpinBox* m_centerY{};
    QDoubleSpinBox* m_topZ{};
    QSpinBox* m_subdivisionsX{};
    QSpinBox* m_subdivisionsY{};
    QCheckBox* m_autoFit{};
    QCheckBox* m_showBlock{};
    QCheckBox* m_showPaths{};
    QCheckBox* m_showTool{};
    QCheckBox* m_flatShading{};
    QCheckBox* m_stockCollisions{};
    QSlider* m_speed{};
    QLabel* m_speedLabel{};
    QLabel* m_statusLabel{};
    QLabel* m_timeLabel{};
    QLabel* m_moveLabel{};
    QProgressBar* m_progress{};

    std::size_t m_lastWarningCount{static_cast<std::size_t>(-1)};
    std::size_t m_lastSuppressedWarnings{static_cast<std::size_t>(-1)};
    bool m_hasShown{false};
};

#endif // CAD_HEADER_SRC_CAD_SRC_GUI_TOOLBARS_CNC_CNC_PANEL_WIDGET_HXX
