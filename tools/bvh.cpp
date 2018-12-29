#include "bvh.h"
#include <functional>
#include <iostream>

using namespace std;

RBVH::RBVH() {
    left_ = nullptr;
    right_ = nullptr;
    min_ = vec3(0,0,0);
    max_ = vec3(0,0,0);
}

void RBVH::Partition(const vector<Shape*>& shapes) {
    if (shapes.size() == 0) {
        cout << "ERROR, TRYING TO PARTITION 0 SHAPES" << endl;
        return;
    }

    // find the bounding box for all shapes combined
    vec3 curr_min, curr_max;
    float minX, maxX, minY, maxY, minZ, maxZ;
    shapes[0]->GetBB(curr_min, curr_max);
    minX = curr_min.x; maxX = curr_max.x;
    minY = curr_min.y; maxY = curr_max.y;
    minZ = curr_min.z; maxZ = curr_max.z;
    vec3 minC = shapes[0]->getCenter();
    vec3 maxC = shapes[0]->getCenter();

    int i = 0;
    for (vector<Shape*>::const_iterator it = shapes.begin(); it != shapes.end(); ++it) {
        // get bounding box for shape
        (*it)->GetBB(curr_min, curr_max);
        minX = min(minX, curr_min.x);
        minY = min(minY, curr_min.y);
        minZ = min(minZ, curr_min.z);
        maxX = max(maxX, curr_max.x);
        maxY = max(maxY, curr_max.y);
        maxZ = max(maxZ, curr_max.z);

        // find the min / max components of all of the shape centers
        vec3 c = (*it)->getCenter();
        minC.x = min(minC.x, c.x);
        minC.y = min(minC.y, c.y);
        minC.z = min(minC.z, c.z);
        maxC.x = max(maxC.x, c.x);
        maxC.y = max(maxC.y, c.y);
        maxC.z = max(maxC.z, c.z);

    }
    min_ = vec3(minX, minY, minZ);
    max_ = vec3(maxX, maxY, maxZ);

    vec3 mid = (minC + maxC) / 2.0f;
    vec3 d = maxC - minC;
    function<bool(const vec3&, const vec3&)> cmp;
    // split on largest axis
    if (d.x >= d.y && d.x >= d.z) {
        cmp = [&](vec3 mid, vec3 center) { return mid.x >= center.x; };
    } else if (d.y >= d.x && d.y >= d.z) {
        cmp = [&](vec3 mid, vec3 center) { return mid.y >= center.y; };
    } else {
        cmp = [&](vec3 mid, vec3 center) { return mid.z >= center.z; };
    }

    vector<Shape*> left;
    vector<Shape*> right;
    i = 0;
    for (vector<Shape*>::const_iterator it = shapes.begin(); it != shapes.end(); ++it) {
        if (cmp(mid, (*it)->getCenter()))
            left.push_back(*it);
        else
            right.push_back(*it);
    }
    if (left.size() == 0 || right.size() == 0) {
        shapes_ = shapes;
        if (left.size() > 1 || right.size() > 1)
            std::cout << "num shapes in leaf: " << left.size() << " " << right.size() << std::endl;
    } else {
        left_ = new RBVH;
        right_ = new RBVH;
        left_->Partition(left);
        right_->Partition(right);
    }
}

int RBVH::count() {
    int sum = 0;
    if (left_)
        sum += left_->count();
    if (right_)
        sum += right_->count();
    return 1 + sum;
}
