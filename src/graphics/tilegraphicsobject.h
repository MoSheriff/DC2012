#ifndef TILEGRAPHICSOBJECT_H
#define TILEGRAPHICSOBJECT_H

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include "graphicsobject.h"
#include "sprites.h"

class TileGraphicsObject
{
public:
    TileGraphicsObject(int type, Position position);
    void draw(GameObject* obj);
    QGraphicsPixmapItem getPixmapItem();
private:
    ~TileGraphicsObject();
    QGraphicsPixmapItem* currentPixmapItem_;
    QPixmap pixmapArray[];
    int type_;
    Point position_;
};

#endif // TILEGRAPHICSOBJECT_H
