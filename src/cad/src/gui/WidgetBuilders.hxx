#ifndef CAD_WIDGETBUILDERS_HXX
#define CAD_WIDGETBUILDERS_HXX

#include <cad_common/WidgetBuilders.hxx>
#include "components/geometry/VirtualPointPropertiesWidget.hpp"

namespace widgets {
    inline VirtualPointPropertiesWidget* addPointProps(QLayout *layout) {
        const auto props = new VirtualPointPropertiesWidget;
        layout->addWidget(props);
        props->setActive(false);
        return props;
    }
}

#endif //CAD_WIDGETBUILDERS_HXX
