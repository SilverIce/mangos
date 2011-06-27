#include <G3D\Vector3.h>

using G3D::Vector3;

inline float hgt(const Map * map, const Vector3& v)
{
    float res = map->GetTerrain()->GetHeight(v.x, v.y, v.z);
    return res > INVALID_HEIGHT ? res : v.z;
}

static bool disable_generation = false;

void GeneratePath(const WorldObject* obj, G3D::Vector3 from, const G3D::Vector3& to, std::vector<G3D::Vector3>& p, bool isFlight)
{
    if (isFlight || disable_generation)
    {
        p.push_back(from);
        p.push_back(to);
    }
    else
    {
        p.push_back(from);

        float length = (to - from).length();
        float step = 4.f;
        int steps = std::max<int>(1, length / step);

        const Map* map = obj->GetMap();
        Vector3 diff = (to - from) / (float)steps;
        Vector3 incr = from;
        int i = 0;
        do
        {
            ++i;
            incr += diff;
            p.push_back(Vector3(incr.x,incr.y, hgt(map,incr)));
        } while(i < steps);
    }
}
