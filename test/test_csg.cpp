#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <test/testutil.h>
#include "test_csg.h"
#include <stdio.h>
#include <cstdint>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;

void TestCSG::setUp()
{
    csg = new Scene();
}

void TestCSG::tearDown()
{
    delete csg;
}

void TestCSG::testSimpleCSG()
{
    // unit tests could be made much more comprehensive than this
    // in particular this fails to test intersection

    csg->sampleScene();
    csg->voxelise(0.1f);
    CPPUNIT_ASSERT(!csg->getVox()->get(100,100,100)); // inside extracted cylinder
    CPPUNIT_ASSERT(!csg->getVox()->get(50,150,100)); // just outside central sphere
    CPPUNIT_ASSERT(csg->getVox()->get(70,100,100)); // just inside central sphere
    CPPUNIT_ASSERT(csg->getVox()->get(150,150,100)); // inside extruding cylinder

    cerr << "CSG SIMPLE SCENE PASSED" << endl << endl;
}

//#if 0 /* Disabled since it crashes the whole test suite */
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TestCSG, TestSet::perBuild());
//#endif
