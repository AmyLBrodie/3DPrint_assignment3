#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <test/testutil.h>
#include "test_mesh.h"
#include <stdio.h>
#include <cstdint>
#include <sstream>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

void TestMesh::setUp()
{
    mesh = new Mesh();
    voxel = new VoxelVolume();
}

void TestMesh::tearDown()
{
    delete mesh;
    delete voxel;
}

void TestMesh::testMeshing()
{

    mesh->readSTL("../meshes/bunny.stl");
    CPPUNIT_ASSERT(mesh->basicValidity());
    CPPUNIT_ASSERT(!mesh->manifoldValidity()); // bunny has known holes in the bottom
    cerr << "BUNNY TEST PASSED" << endl << endl;

    // test simple valid 2-manifold
    mesh->validTetTest();
    CPPUNIT_ASSERT(mesh->basicValidity());
    CPPUNIT_ASSERT(mesh->manifoldValidity());
    cerr << "SIMPLE VALIDITY TEST PASSED" << endl << endl;

    // test for duplicate vertices, dangling vertices and out of bounds on vertex indices
    mesh->basicBreakTest();
    CPPUNIT_ASSERT(!mesh->basicValidity());
    cerr << "BASIC INVALID MESH DETECTED CORRECTLY" << endl << endl;

    // test for 2-manifold with boundary
    mesh->openTetTest();
    CPPUNIT_ASSERT(mesh->basicValidity());
    CPPUNIT_ASSERT(!mesh->manifoldValidity());
    cerr << "INVALID MESH WITH BOUNDARY DETECTED CORRECTLY" << endl << endl;

    // test for non 2-manifold failure where surfaces touch at a single vertex
    mesh->touchTetsTest();
    CPPUNIT_ASSERT(mesh->basicValidity());
    CPPUNIT_ASSERT(!mesh->manifoldValidity());
    cerr << "INVALID PINCHED SURFACE TEST PASSED" << endl << endl;

    // test for non 2-manifold overlapping triangles
    mesh->overlapTetTest();
    CPPUNIT_ASSERT(mesh->basicValidity());
    CPPUNIT_ASSERT(!mesh->manifoldValidity());
    cerr << "INVALID NON-2-MANIFOLD DETECTED CORRECTLY" << endl << endl;

}

void TestMesh::testMarchingCubes(){
	mesh->readSTL("../meshes/cube.stl");
	int v, dx, dy, dz;

    dx = dy = dz = 1024;
    voxel->setDim(dx, dy, dz); // typical large voxel volume
    voxel->setFrame(cgp::Point(0.0f, 0.0f, 0.0f), cgp::Vector(1.0f, 1.0f, 1.0f)); // unit cube

    srand (time(NULL));

    // check fill at random positions
    voxel->fill(false);
    for(v = 0; v < 10; v++)
    {
        // check random positions in voxel volume
        CPPUNIT_ASSERT(voxel->getMCVertIdx(rand()%1024,rand()%1024,rand()%1024)==0);
    }
    
    voxel->fill(true);
    for(v = 0; v < 10; v++)
    {
        // check random positions in voxel volume
        CPPUNIT_ASSERT(voxel->getMCVertIdx(rand()%1024,rand()%1024,rand()%1024)!=0);
    }
    
    cerr << "VERTEX BIT INDEX TEST PASSED" << endl;
    
    float voxelLen = 5.0f;
    cgp::Vector volDiagonal(10.0f, 10.0f, 10.0f);
    int dimX = ceil(volDiagonal.i/voxelLen) +2;
    int dimY = ceil(volDiagonal.j/voxelLen) +2;
    int dimZ = ceil(volDiagonal.k/voxelLen) +2;
    cgp::Vector voxelDiagonal = cgp::Vector((float)dimX*voxelLen, (float)dimY*voxelLen, (float)dimZ*voxelLen);
    cgp::Point voxelOrigin = cgp::Point(-0.5f*voxelDiagonal.i, -0.5*voxelDiagonal.j, -0.5*voxelDiagonal.k);
    
    voxel->setDim(dimX,dimY,dimZ);
    voxel->setFrame(voxelOrigin, voxelDiagonal);
    voxel->fill(false);
    
    mesh->marchingCubes(*voxel);
    CPPUNIT_ASSERT(mesh->getTris().size() == 0);
    CPPUNIT_ASSERT(mesh->getVerts().size() == 0);
    
    VoxelVolume * voxel1 = new VoxelVolume();
    voxel1->setDim(dimX,dimY,dimZ);
    voxel1->setFrame(voxelOrigin, voxelDiagonal);
    voxel1->fill(false);
    voxel1->set(0,0,0,true);
    
    mesh->marchingCubes(*voxel1);
    CPPUNIT_ASSERT(mesh->getTris().size() != 0);
    CPPUNIT_ASSERT(mesh->getTris().size() == 1);
    CPPUNIT_ASSERT(mesh->getVerts().size() != 0);
    
    cerr << "MARCHING CUBES TEST PASSED" << endl;
}

//#if 0 /* Disabled since it crashes the whole test suite */
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TestMesh, TestSet::perBuild());
//#endif
