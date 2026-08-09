//
// Created by Radosław Głasek on 08.08.2026
//

#ifndef CAD_DIALOGCARD_HXX
#define CAD_DIALOGCARD_HXX

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QVBoxLayout>

#include "CadTitleBar.hpp"
#include "Theme.hpp"

/// @brief Builders for the shared frameless-card dialog
namespace widgets {
    /// @brief Dress @p dialog as a frameless, shadowed card with a title bar
    /// and return the form to put its content into
    /// @details Frameless with drag from the title bar; translucent window so
    /// the rounded card corners and drop shadow render. The dialog is locked to
    /// its layout's size
    inline QFormLayout* buildDialogCard(QDialog *dialog, const QString &title) {
        dialog->setWindowTitle(title);
        dialog->setWindowFlag(Qt::FramelessWindowHint);
        dialog->setAttribute(Qt::WA_TranslucentBackground);
        dialog->setStyleSheet(theme::dialogCardStyle(theme::active()));

        const auto shell = new QVBoxLayout(dialog);
        shell->setContentsMargins(12, 12, 12, 12);

        const auto card = new QWidget(dialog);
        card->setObjectName("dialogCard");
        const auto shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(24);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(0, 0, 0, 90));
        card->setGraphicsEffect(shadow);
        shell->addWidget(card);

        const auto outer = new QVBoxLayout(card);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);
        const auto titleBar = new DialogTitleBar(title);
        titleBar->setAutoFillBackground(false); // QSS above paints the tinted strip
        titleBar->setAttribute(Qt::WA_StyledBackground); // plain QWidget needs this for QSS backgrounds
        outer->addWidget(titleBar);

        const auto form = new QFormLayout;
        form->setContentsMargins(14, 12, 14, 14);
        form->setHorizontalSpacing(12);
        form->setVerticalSpacing(8);
        outer->addLayout(form);

        dialog->setSizeGripEnabled(false);
        shell->setSizeConstraint(QLayout::SetFixedSize);
        return form;
    }

    /// @brief Append the standard Ok/Cancel row wired to accept/reject
    inline QDialogButtonBox* addDialogButtons(const QDialog *dialog, QFormLayout *form) {
        const auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttons->button(QDialogButtonBox::Ok)->setObjectName("okButton");
        buttons->button(QDialogButtonBox::Cancel)->setObjectName("cancelButton");
        form->addRow(buttons);
        QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
        return buttons;
    }
}

#endif //CAD_DIALOGCARD_HXX
