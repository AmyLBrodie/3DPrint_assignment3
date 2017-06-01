//
// ffd
//

#include "ffd.h"
#include <stdio.h>

using namespace std;

GLfloat defaultLatCol[] = {0.2f, 0.2f, 0.2f, 1.0f};
GLfloat highlightLatCol[] = {1.0f, 0.176f, 0.176f, 1.0f};
int maxbezorder = 4;

void ffd::alloc()
{
    // allocate memory for a 3D array of control points and highlighting switches
    if(dimx > 1 && dimy > 1 && dimz > 1 && dimx <= maxbezorder && dimy <= maxbezorder && dimz <= maxbezorder)
    {
        cp = new cgp::Point **[dimx];
        highlight = new bool **[dimx];
        for (int i = 0; i < dimx; i++)
        {
            cp[i] = new cgp::Point *[dimy];
            highlight[i] = new bool *[dimy];

            for (int j = 0; j < dimy; j++)
            {
                cp[i][j] = new cgp::Point[dimz];
                highlight[i][j] = new bool[dimz];
            }
        }
        deactivateAllCP();
    }
}

void ffd::dealloc()
{
    // deallocate 3D array of control points and boolean highlighting switches
    for (int i = 0; i < dimx; i++)
    {
        for (int j = 0; j < dimy; j++)
        {
            delete [] cp[i][j];
            delete [] highlight[i][j];
        }

        delete [] cp[i];
        delete [] highlight[i];
    }
    delete [] cp;
    delete [] highlight;
    cp = NULL;
}

bool ffd::inCPBounds(int i, int j, int k)
{
    return (i >= 0 && j >= 0 && k >= 0 && i < dimx && j < dimy && k < dimz);
}

ffd::ffd()
{
    dimx = dimy = dimz = 0;
    setFrame(cgp::Point(0.0f, 0.0f, 0.0f), cgp::Vector(0.0f, 0.0f, 0.0f));
    cp = NULL;
    highlight = NULL;
}

ffd::ffd(int xnum, int ynum, int znum, cgp::Point corner, cgp::Vector diag)
{
    dimx = xnum;
    dimy = ynum;
    dimz = znum;
    alloc();
    setFrame(corner, diag);
}

void ffd::reset()
{
    float val = 0.0f;
    
    float pos1 = dimx-1;
    float pos2 = dimy-1;
    float pos3 = dimz-1;
    
    // creates S,T and U vectors
    cgp::Vector SVector(diagonal.i,val,val);
    cgp::Vector TVector(val,diagonal.j,val);
    cgp::Vector UVector(val,val,diagonal.k);
    
    
    // resets position of control points in lattice
    for (int i=0; i<dimx; i++){
    	for (int j=0; j<dimy; j++){
    		for (int k=0; k<dimz; k++){
    			cp[i][j][k] = origin;
    			cp[i][j][k].x = ((i/pos1) * SVector.i) + ((j/pos2) * TVector.i) + ((k/pos3) * UVector.i);
    			cp[i][j][k].y = ((i/pos1) * SVector.j) + ((j/pos2) * TVector.j) + ((k/pos3) * UVector.j);
    			cp[i][j][k].z = ((i/pos1) * SVector.k) + ((j/pos2) * TVector.k) + ((k/pos3) * UVector.k);
    		}
    	}
    }
}

void ffd::getDim(int &numx, int &numy, int &numz)
{
    numx = dimx; numy = dimy; numz = dimz;
}

void ffd::setDim(int numx, int numy, int numz)
{
    dimx = numx; dimy = numy; dimz = numz;
    alloc();
    reset();
}

void ffd::getFrame(cgp::Point &corner, cgp::Vector &diag)
{
    corner = origin;
    diag = diagonal;
}

void ffd::setFrame(cgp::Point corner, cgp::Vector diag)
{
    origin = corner;
    diagonal = diag;
    reset();
}

void ffd::activateCP(int i, int j, int k)
{
    if(inCPBounds(i,j,k))
        highlight[i][j][k] = true;
}

void ffd::deactivateCP(int i, int j, int k)
{
    if(inCPBounds(i,j,k))
        highlight[i][j][k] = false;
}

void ffd::deactivateAllCP()
{
    for(int i = 0; i < dimx; i++)
        for(int j = 0; j < dimy; j++)
            for(int k = 0; k < dimz; k++)
                highlight[i][j][k] = false;
}

bool ffd::bindGeometry(View * view, ShapeDrawData &sdd, bool active)
{
    int i, j, k;
    glm::mat4 tfm, idt;
    glm::vec3 trs;
    cgp::Point pnt;
    bool draw;

    if(active)
    {
        activegeom.clear();
        activegeom.setColour(highlightLatCol);
    }
    else
    {
        geom.clear();
        geom.setColour(defaultLatCol);
    }

    idt = glm::mat4(1.0f); // identity matrix

    // place a sphere at non-active control point positions with appropriate colour
    for(i = 0; i < dimx; i++)
        for(j = 0; j < dimy; j++)
            for(k = 0; k < dimz; k++)
            {
                if(active) // only draw those control points that match active flag
                    draw = highlight[i][j][k];
                else
                    draw = !highlight[i][j][k];

                if(draw)
                {
                    pnt = cp[i][j][k];
                    trs = glm::vec3(pnt.x, pnt.y, pnt.z);
                    tfm = glm::translate(idt, trs);
                    if(active)
                        activegeom.genSphere(0.4, 10, 10, tfm);
                    else
                        geom.genSphere(0.4, 10, 10, tfm);
                }
            }

    // bind geometry to buffers and return drawing parameters, if possible
    if(active)
    {
        if(activegeom.bindBuffers(view))
        {
            sdd = activegeom.getDrawParameters();
            return true;
        }
        else
            return false;
    }
    else
    {
        if(geom.bindBuffers(view))
        {
            sdd = geom.getDrawParameters();
            return true;
        }
        else
            return false;
    }
}

cgp::Point ffd::getCP(int i, int j, int k)
{
    if(inCPBounds(i,j,k))
    {
        return cp[i][j][k];
    }
    else
    {
        cerr << "Error ffd::getCP: out of bounds access to lattice" << endl;
        return cgp::Point(0.0f, 0.0f, 0.0f);
    }
}

void ffd::setCP(int i, int j, int k, cgp::Point pnt)
{
    if(inCPBounds(i,j,k))
        cp[i][j][k] = pnt;
}

int chooseRecurse(int i, int j){
	if (j == 0){
		return 1;
	}
	else{
		return (i * chooseRecurse(i-1, j-1)) / j;
	}
}

// so far it just places the shape in the lattice, I couldn't figure out the rest
void ffd::deform(cgp::Point & pnt)
{
    float val = 0.0f;
    
    // finds difference between the origin and a point
    cgp::Vector difference;
    difference.diff(origin, pnt);
    
    // Creates the S,T and U vectors
    cgp::Vector SVector(diagonal.i, val, val);
    cgp::Vector TVector(val, diagonal.j, val);
    cgp::Vector UVector(val, val, diagonal.k);
    
    // finds the cross product of T and U
    cgp::Vector crossTU;
    crossTU.cross(TVector,UVector);
    float s = (crossTU.dot(difference))/(crossTU.dot(SVector));
    
    // finds the cross product of S and T
    cgp::Vector crossST;
    crossST.cross(SVector,TVector);
    float u = (crossST.dot(difference))/(crossST.dot(UVector));
    
    // finds the cross product of S and U
    cgp::Vector crossSU;
    crossSU.cross(SVector,UVector);
    float t = (crossSU.dot(difference))/(crossSU.dot(TVector));
    
    
    // gives influence of control points in lattice
    
    cgp::Point sumOfX(val, val, val); // intialise point for x influence
    for (int i=0; i<dimx; i++){
    	cgp::Point sumOfY(val, val, val); // intialise point for y influence
    	for (int j=0; j<dimx; j++){
    		cgp::Point sumOfZ(val, val, val); // intialise point for z influence
    		for (int k=0; k<dimx; k++){
    			float z = chooseRecurse(dimz-1,k) * pow(1-u,dimz-1-k) * pow(u,k);
    			
    			sumOfZ.x += z * cp[i][j][k].x;
    			sumOfZ.y += z * cp[i][j][k].y;
    			sumOfZ.z += z * cp[i][j][k].z;
    		}
    		float y = chooseRecurse(dimy-1,j) * pow(1-t,dimy-1-j) * pow(t,j);
    		
    		sumOfY.x += y * sumOfZ.x;
			sumOfY.y += y * sumOfZ.y;
			sumOfY.z += y * sumOfZ.z;
    	}
    	float x = chooseRecurse(dimx-1, i) * pow(1-s,dimx-1-i) * pow(s,i);
    	
    	sumOfX.x += x * sumOfY.x;
		sumOfX.y += x * sumOfY.y;
		sumOfX.z += x * sumOfY.z;
    }
    pnt = sumOfX;
}
