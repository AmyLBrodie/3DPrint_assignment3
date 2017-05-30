//
// csg
//

#include "csg.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <limits>
#include <stack>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

using namespace std;
// using namespace cgp;

GLfloat defaultCol[] = {0.243f, 0.176f, 0.75f, 1.0f};

bool Scene::genVizRender(View * view, ShapeDrawData &sdd)
{
    std::vector<ShapeNode *> leaves;
    std::stack<SceneNode *> nodes;
    SceneNode * currnode;
    OpNode * currop;
    ShapeNode * currshape;
    int i;

    geom.clear();
    geom.setColour(defaultCol);

    // gather vector of leaf nodes
    if(csgroot != NULL)
    {
        if(dynamic_cast<ShapeNode*>( csgroot ) == 0) // OpNode
        {
            nodes.push(csgroot);
        }
        else // ShapeNode
        {
            currshape = dynamic_cast<ShapeNode*>( csgroot );
            leaves.push_back((ShapeNode *) csgroot);
        }

        // explicit walk of csg tree using stack
        while(!nodes.empty())
        {
            currnode = nodes.top();
            currop = dynamic_cast<OpNode*> (currnode);
            nodes.pop();

            // left subtree
            if(dynamic_cast<ShapeNode*>( currop->left ) == 0) // cast fails so it is an OpNode
            {
                nodes.push(currop->left);
            }
            else // ShapeNode, so treat as leaf
            {
                currshape = dynamic_cast<ShapeNode*>( currop->left );
                leaves.push_back(currshape);
            }

            // right subtree
            if(dynamic_cast<ShapeNode*>( currop->right ) == 0) // cast fails so it is an OpNode
            {
                nodes.push(currop->right);
            }
            else // ShapeNode, so treat as leaf
            {
                currshape = dynamic_cast<ShapeNode*>( currop->right );
                leaves.push_back(currshape);
            }
        }
    }

    // traverse leaf shapes generating geometry
    for(i = 0; i < (int) leaves.size(); i++)
    {
        leaves[i]->shape->genGeometry(&geom, view);
    }

    // bind geometry to buffers and return drawing parameters, if possible
    if(geom.bindBuffers(view))
    {
        sdd = geom.getDrawParameters();
        return true;
    }
    else
        return false;
}

bool Scene::genVoxRender(View * view, ShapeDrawData &sdd)
{
    int x, y, z, xdim, ydim, zdim;
    glm::mat4 tfm, idt;
    glm::vec3 trs;
    cgp::Point pnt;

    geom.clear();
    geom.setColour(defaultCol);

    if(rep == SceneRep::VOXELS)
    {
        idt = glm::mat4(1.0f); // identity matrix

        vox.getDim(xdim, ydim, zdim);

        // place a sphere at filled voxels but subsample to avoid generating too many spheres
        for(x = 0; x < xdim; x+=10)
            for(y = 0; y < ydim; y+=10)
                for(z = 0; z < zdim; z+=10)
                {
                    if(vox.get(x, y, z))
                    {
                        pnt = vox.getVoxelPos(x, y, z); // convert from voxel space to world coordinates
                        trs = glm::vec3(pnt.x, pnt.y, pnt.z);
                        tfm = glm::translate(idt, trs);
                        geom.genSphere(voxsidelen * 5.0f, 3, 3, tfm);
                    }
                }

    }

    // bind geometry to buffers and return drawing parameters, if possible
    if(geom.bindBuffers(view))
    {
        sdd = geom.getDrawParameters();
        return true;
    }
    else
        return false;
}

Scene::Scene()
{
    csgroot = NULL;
    col = defaultCol;
    voldiag = cgp::Vector(20.0f, 20.0f, 20.0f);
    voxsidelen = 0.0f;
    rep = SceneRep::TREE;
}

Scene::~Scene()
{
    clear();
}

void Scene::clear()
{
    std::stack<SceneNode *> nodes;
    SceneNode * currnode;
    OpNode * currop;
    ShapeNode * currleaf;

    geom.clear();
    vox.clear();

    // walk csg tree and deallocate nodes
    if(csgroot != NULL)
    {
        nodes.push(csgroot);

        // explicit walk of csg tree using stack
        while(!nodes.empty())
        {
            currnode = nodes.top();
            nodes.pop(); // calls destructor

            if(dynamic_cast<OpNode*> (currnode))
            {
                currop = dynamic_cast<OpNode*> (currnode);
                nodes.push(currop->right);
                nodes.push(currop->left);
                delete currop;
            }
            else
            {
                if(dynamic_cast<ShapeNode*> (currnode))
                {
                    currleaf = dynamic_cast<ShapeNode*> (currnode);
                    delete currleaf;
                }
                else
                {
                    cerr << "Error Scene::clear(): CSG tree is not properly formed" << endl;
                }
            }
        }
    }
}

bool Scene::bindGeometry(View * view, ShapeDrawData &sdd)
{
    bool pass;

    // call different geometric construction methods depending on current representation
    switch(rep)
    {
        case SceneRep::TREE:
            pass = genVizRender(view, sdd);
            break;
        case SceneRep::VOXELS:
            pass = genVoxRender(view, sdd);
            break;
        case SceneRep::ISOSURFACE:
            pass = voxmesh.bindGeometry(view, sdd);
            break;
        default:
            pass = false;
            break;
    }
    return pass;
}

void Scene::voxSetOp(SetOp op, VoxelVolume *leftarg, VoxelVolume *rightarg)
{
    /*
     switch based on op
     UNION: wherever voxel is set in rightarg copy to leftarg
     INTERSECTION: if voxel is set in leftarg, check to see if it is also set in rightarg, otherwise switch it off
     DIFFERENCE: wherever voxel is set in rightarg turn it off in leftarg
     */

    int x, y, z, dx1, dx2, dy1, dy2, dz1, dz2;

    // check that volume sizes match
    leftarg->getDim(dx1, dy1, dz1);
    rightarg->getDim(dx2, dy2, dz2);
    if(dx1 != dx2 || dy1 != dy2 || dz1 != dz2)
    {
    }
    else
    {
        switch(op)
        {
            case SetOp::UNION: // wherever voxel is set in rightarg copy to leftarg
                for(x = 0; x < dx1; x++)
                    for(y = 0; y < dy1; y++)
                        for(z = 0; z < dz1; z++)
                            if(rightarg->get(x,y,z))
                                leftarg->set(x,y,z,true);
                break;
            case SetOp::INTERSECTION: // if voxel is set in leftarg, check to see if it is also set in rightarg, otherwise switch it off
                for(x = 0; x < dx1; x++)
                    for(y = 0; y < dy1; y++)
                        for(z = 0; z < dz1; z++)
                            if(leftarg->get(x,y,z))
                                if(!rightarg->get(x,y,z))
                                    leftarg->set(x,y,z,false);
                break;
            case SetOp::DIFFERENCE: // wherever voxel is set in rightarg turn it off in leftarg
                for(x = 0; x < dx1; x++)
                    for(y = 0; y < dy1; y++)
                        for(z = 0; z < dz1; z++)
                            if(rightarg->get(x,y,z))
                                leftarg->set(x,y,z,false);
                break;
            default:
                break;
        }
    }
}

void Scene::voxWalk(SceneNode *root, VoxelVolume *voxels)
{
    // traverse csg tree by depth first recursive walk
    /*
     if(root is leaf)
     convert to voxel rep, store in voxels
     else
     voxWalk left tree (leftnode, voxels)
     allocate rightvoxels, copy parameters from voxels
     voxWalk right tree (rightnode, rightvoxels)
     apply opp to voxels and rightvoxels store results in voxels
     deallocate rightvoxels
     */

    VoxelVolume * rightvoxels;
    ShapeNode * shapenode;
    OpNode * opnode;
    int dx, dy, dz;
    cgp::Point o;
    cgp::Vector d;

    if(dynamic_cast<ShapeNode*>( root )) // ShapeNode
    {
        shapenode = dynamic_cast<ShapeNode*>( root );
        // efficiency could be improved by only considering bounding box area
        voxels->getDim(dx, dy, dz);
        for(int x = 0; x < dx; x++)
        {
#pragma omp parallel for
            for(int y = 0; y < dy; y++)
                for(int z = 0; z < dz; z++)
                    voxels->set(x,y,z, shapenode->shape->pointContainment(voxels->getVoxelPos(x,y,z)));
        }
    }
    else // OpNode
    {
        if(dynamic_cast<OpNode*>( root )) // Sanity check in case something is wrong with the tree
        {
            // as pointed out by Mark this could have memory issues with the wrong kind of unbalanced tree
            // could be fixed if memory allocation for voxel volume where delayed to leaf nodes
            opnode = dynamic_cast<OpNode*>( root );
            voxWalk(opnode->left, voxels);
            voxels->getDim(dx, dy, dz);
            voxels->getFrame(o, d);
            rightvoxels = new VoxelVolume(dx, dy, dz, o, d);
            voxWalk(opnode->right, rightvoxels);
            voxSetOp(opnode->op, voxels, rightvoxels);
            delete rightvoxels;
        }
        else
        {
            cerr << "Error Scene::voxWalk: csg tree is not properly formed" << endl;
        }
    }
}

void Scene::voxelise(float voxlen)
{
    int xdim, ydim, zdim;

    // calculate voxel volume dimensions based on voxlen
    xdim = ceil(voldiag.i / voxlen)+2; // needs a 1 voxel border to ensure a closed mesh
    ydim = ceil(voldiag.j / voxlen)+2;
    zdim = ceil(voldiag.k / voxlen)+2;
    voxsidelen = voxlen;

    cgp::Vector voxdiag = cgp::Vector((float) xdim * voxlen, (float) ydim * voxlen, (float) zdim * voxlen);
    cgp::Point voxorigin = cgp::Point(-0.5f*voxdiag.i, -0.5f*voxdiag.j, -0.5f*voxdiag.k);
    vox.setDim(xdim, ydim, zdim);
    vox.setFrame(voxorigin, voxdiag);

    cerr << "Voxel volume dimensions = " << xdim << " x " << ydim << " x " << zdim << endl;

    // actual recursive depth-first walk of csg tree
    if(csgroot != NULL)
        voxWalk(csgroot, &vox);
    rep = SceneRep::VOXELS;
}

void Scene::isoextract()
{
    voxmesh.marchingCubes(vox);
    rep = SceneRep::ISOSURFACE;
}

void Scene::smooth()
{
    voxmesh.laplacianSmooth(6, 1.0f);
}

void Scene::deform(ffd * def)
{
    voxmesh.applyFFD(def);
}

void Scene::sampleScene()
{
    ShapeNode * sph = new ShapeNode();
    sph->shape = new Sphere(cgp::Point(0.0f, 0.0f, 0.0f), 4.0f);
    //sph->shape = new Cylinder(cgp::Point(0.0f, 0.0f, -7.0f), cgp::Point(0.0f, 0.0f, 7.0f), 1.5f);

    ShapeNode * cyl1 = new ShapeNode();
    cyl1->shape = new Cylinder(cgp::Point(-7.0f, -7.0f, 0.0f), cgp::Point(7.0f, 7.0f, 0.0f), 2.0f);

    ShapeNode * cyl2 = new ShapeNode();
    cyl2->shape = new Cylinder(cgp::Point(0.0f, -7.0f, 0.0f), cgp::Point(0.0f, 7.0f, 0.0f), 2.5f);

    OpNode * combine = new OpNode();
    combine->op = SetOp::UNION;
    combine->left = cyl1;
    combine->right = sph;

    OpNode * diff = new OpNode();
    diff->op = SetOp::DIFFERENCE;
    diff->left = combine;
    diff->right = cyl2;

    csgroot = diff;
}

void Scene::expensiveScene()
{
    ShapeNode * sph = new ShapeNode();
    sph->shape = new Sphere(cgp::Point(1.0f, -2.0f, -2.0f), 3.0f);

    ShapeNode * cyl = new ShapeNode();
    cyl->shape = new Cylinder(cgp::Point(-7.0f, -7.0f, 0.0f), cgp::Point(7.0f, 7.0f, 0.0f), 2.0f);

    ShapeNode * mesh = new ShapeNode();
    Mesh * bunny = new Mesh();
    bunny->readSTL("../meshes/bunny.stl");
    bunny->boxFit(10.0f);
    mesh->shape = bunny;

    OpNode * combine = new OpNode();
    combine->op = SetOp::UNION;
    combine->left = mesh;
    combine->right = cyl;

    OpNode * diff = new OpNode();
    diff->op = SetOp::DIFFERENCE;
    diff->left = combine;
    diff->right = mesh;

    csgroot = diff;
}