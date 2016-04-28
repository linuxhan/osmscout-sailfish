/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef INPUTHANDLER_H
#define	INPUTHANDLER_H

#include <QObject>
#include <QVector2D>
#include <QTouchEvent>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>

struct MapView
{
  osmscout::GeoCoord           center;
  double                       angle;
  osmscout::Magnification      magnification;
};

Q_DECLARE_METATYPE(MapView)

/**
 * Input handler retrieve all inputs from user and may change MapView. 
 * If handler don't accept specific action, returns false. In such case, 
 * default handler for this action should be activated.
 * 
 * Input handlers is application of behaviour pattern. It solves problems like: 
 *  - what should happen when finger is on the screen and plus button is pressed
 *  - recognising multitouch gestures
 *      Qt provides api for register custom gesture recognizers, but it is not 
 *      available in QML world and its api don't fit to Map application requierements.
 * 
 * Handler may control map animations in future.
 */

class InputHandler : public QObject{
    Q_OBJECT
public:
    InputHandler(MapView view);
    virtual ~InputHandler();

    virtual void painted();
    virtual bool animationInProgress();
    
    virtual bool showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification);
    virtual bool zoomIn(double zoomFactor); // TODO: merge zoom in/out to one method
    virtual bool zoomOut(double zoomFactor);
    virtual bool move(QVector2D vector); // move vector in pixels
    virtual bool rotateBy(double angleStep, double angleChange);
    virtual bool touch(QTouchEvent *event);

signals:
    void viewChanged(const MapView &view);

protected:
    MapView view;
};

class MoveHandler : public InputHandler {
    Q_OBJECT
public: 
    MoveHandler(MapView view, double dpi);
    virtual ~MoveHandler();

    virtual bool zoomIn(double zoomFactor);
    virtual bool zoomOut(double zoomFactor);
    virtual bool move(QVector2D vector); // move vector in pixels
    virtual bool rotateBy(double angleStep, double angleChange);
private:
    double dpi;
};

class DragHandler : public MoveHandler {
    Q_OBJECT
public: 
    DragHandler(MapView view, double dpi);
    virtual ~DragHandler();

    virtual bool animationInProgress();

    virtual bool zoomIn(double zoomFactor);
    virtual bool zoomOut(double zoomFactor);
    virtual bool move(QVector2D vector); // move vector in pixels
    virtual bool rotateBy(double angleStep, double angleChange);
    
    virtual bool touch(QTouchEvent *event);
private:
    bool moving;
    MapView startView;
    int fingerId;
    int startX;
    int startY;
};

class MultitouchHandler : public MoveHandler {
    Q_OBJECT
public: 
    MultitouchHandler(MapView view, double dpi);
    virtual ~MultitouchHandler();

    virtual bool animationInProgress();

    virtual bool zoomIn(double zoomFactor);
    virtual bool zoomOut(double zoomFactor);
    virtual bool move(QVector2D vector); // move vector in pixels
    virtual bool rotateBy(double angleStep, double angleChange);
    
    virtual bool touch(QTouchEvent *event);
private:
    bool moving;
    MapView startView;
    bool initialized;
    
    // we take only first two touch points into account
    QTouchEvent::TouchPoint startPointA;
    QTouchEvent::TouchPoint startPointB;
};



#endif	/* INPUTHANDLER_H */


