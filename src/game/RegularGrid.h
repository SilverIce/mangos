#pragma once

#include "G3D\Ray.h"
#include "G3D\AABox.h"
#include "G3D\Table.h"
#include "G3D\BoundsTrait.h"
#include "G3D\PositionTrait.h"

#include "Errors.h"

using G3D::Vector2;
using G3D::Vector3;
using G3D::AABox;
using G3D::Ray;

template<class Node>
struct NodeCreator{
    static Node * makeNode(int x, int y) { return new Node();}
};

template<class T,
class Node,
class NodeCreatorFunc = NodeCreator<Node>,
/*class BoundsFunc = BoundsTrait<T>,*/
class PositionFunc = PositionTrait<T>
>
class RegularGrid2D
{
public:
    enum{
        CELL_NUMBER = 64,
        CELL_LAST = CELL_NUMBER - 1,
        TREE_SIZE = CELL_NUMBER*CELL_NUMBER,
    };
    #define CELL_SIZE double(533.3333333333333333333)
    #define HGRID_MAP_SIZE (CELL_SIZE * CELL_NUMBER)

    typedef Array<const T*> MemberArray;
    typedef G3D::Table<const T*, Node*> MemberTable;

    MemberTable memberTable;
    Node * nodes[CELL_NUMBER][CELL_NUMBER];

    RegularGrid2D(){
        memset(nodes, 0, sizeof nodes);
    }

    ~RegularGrid2D(){
        for (int x = 0; x < CELL_NUMBER; ++x)
            for (int y = 0; y < CELL_NUMBER; ++y)
                delete nodes[x][y];
    }

    void insert(const T& value)
    {
        Vector3 pos;
        PositionFunc::getPosition(value, pos);
        Node& node = getGridFor(pos.x, pos.y);
        node.insert(value);
        memberTable.set(&value, &node);
    }

    void remove(const T& value)
    {
        memberTable[&value]->remove(value);
        // Remove the member
        memberTable.remove(&value);
    }

    void balance()
    {
        for (int x = 0; x < CELL_NUMBER; ++x)
            for (int y = 0; y < CELL_NUMBER; ++y)
                if (Node * n = nodes[x][y])
                    n->balance();
    }

    bool contains(const T& value) const { return memberTable.containsKey(&value);}
    int size() const { return memberTable.size();}

    struct Cell
    {
        int x, y;
    };

    static Cell ComputeCell(float fx, float fy)
    {
        Cell c = {fx / CELL_SIZE + (CELL_NUMBER/2), fy / CELL_SIZE + (CELL_NUMBER/2)};
        return c;
    }

    Node& getGridFor(float fx, float fy)
    {
        Cell c = ComputeCell(fx, fy);
        return getGrid(c.x, c.y);
    }

    Node& getGrid(int x, int y)
    {
        MANGOS_ASSERT(x < CELL_NUMBER && y < CELL_NUMBER);
        if (!nodes[x][y])
            nodes[x][y] = NodeCreatorFunc::makeNode(x,y);
        return *nodes[x][y];
    }

    template<typename RayCallback>
    void intersectRay(const Ray& ray, RayCallback& intersectCallback, float max_dist)
    {
        float v = (float)CELL_SIZE;

        float kx = ray.direction().x, bx = ray.origin().x;
        float ky = ray.direction().y, by = ray.origin().y;

        float dt;
        if (fabs(kx) > fabs(ky))
        {
            if (G3D::fuzzyNe(ky,0))
                dt = v / ky * 1.00001;
            else
                dt = v / kx * 1.00001;
        }
        else
        {
            if (G3D::fuzzyNe(kx,0))
                dt = v / kx * 1.00001;
            else
                dt = v / ky * 1.00001;
        }

        int i = 0;
        int N = ceilf(max_dist / fabs(dt));

        // precomputed vars:
        float beg_x = bx/v + CELL_NUMBER/2;
        float beg_y = by/v + CELL_NUMBER/2;
        if (N == 1) // optimize most common case
        {
            int x = int(beg_x);
            int y = int(beg_y);
            float enter_dist = max_dist;
            if (Node * node = nodes[x][y])
                node->intersectRay(ray, intersectCallback, enter_dist);
            return;
        }

        // precomputed vars:
        float dx = dt * kx / v;
        float dy = dt * ky / v;
        do
        {
            int x = int(beg_x);
            int y = int(beg_y);
            if (Node * node = nodes[x][y])
            {
                float enter_dist = max_dist;
                node->intersectRay(ray, intersectCallback, enter_dist);
                if (intersectCallback.didHit())
                    return;
            }

            beg_x += dx;
            beg_y += dy;
            ++i;
        } while (i < N);
    }
};

#undef CELL_SIZE
#undef HGRID_MAP_SIZE
#undef CELL_SIZE
