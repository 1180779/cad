//
// Created on 3/6/26.
//

#ifndef CAD_DOUBLESLIDER_H
#define CAD_DOUBLESLIDER_H

#include <QSlider>
#include <QObject>

//
// inspired by stack overflow
// https://stackoverflow.com/questions/19003369/how-to-make-a-qslider-change-with-double-values
//

class DoubleSlider : public QSlider
{
    Q_OBJECT

public:
    explicit DoubleSlider(QWidget *parent = nullptr);
    void setMappingRange(double start, double end);
    void setValue(double value);
    double value() const;

    signals  :

    
    void doubleValueChanged(double value);

public
    slots  :

    
    void notifyValueChanged(int value);

private:
    double m_mappingRangeStart = 0.0;
    double m_mappingRangeEnd = 1.0;
    double m_mappingRangeLength = 1.0;
};


#endif //CAD_DOUBLESLIDER_H