#pragma once

namespace G3D
{
    class Vector3;
}

namespace VMAP
{
    class ModelInstance;
}

using G3D::Vector3;
using VMAP::ModelInstance;

class DynamicMapTree
{
    struct KDTreeTest& impl;
public:

    DynamicMapTree();
    ~DynamicMapTree();

    bool isInLineOfSight(const Vector3& pos1, const Vector3& pos2) const;
    bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2);

    void insert(const ModelInstance&);
    void remove(const ModelInstance&);
    void contains(const ModelInstance&);
    void clear();
    int size() const;

    void balance(int valuesPerNode = 5, int numMeanSplits = 3);

    int unbalanced_times;
};

