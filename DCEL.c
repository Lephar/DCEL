#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define GROWTH_COEFFICIENT 2	//ArrayList growth rate when full

struct Vertex {
	double x;
	double y;
	struct HalfEdge *IncidentEdge;
	struct DCEL *d;	//Needed for outer face access in getCommonFace if there is no IncidentEdge
};

struct HalfEdge {
	struct Vertex *Origin;
	struct HalfEdge *TwinEdge;
	struct Face *IncidentFace;
	struct HalfEdge *NextEdge;
	struct HalfEdge *PreviousEdge;
};

struct Face {
	struct HalfEdge *IncidentEdge;
};

struct DCEL {	//All data types are stored in ArrayLists but they are not really used
	struct Vertex **Vertices;
	struct HalfEdge **Edges;
	struct Face **Faces;
	int NumberOfVertices;
	int NumberOfEdges;
	int NumberOfFaces;
	int CapacityOfVertices;
	int CapacityOfEdges;
	int CapacityOfFaces;
};

typedef struct Vertex Vertex;
typedef struct HalfEdge HalfEdge;
typedef struct Face Face;
typedef struct DCEL DCEL;

DCEL *makeDCEL() {
	DCEL *d = malloc(sizeof(DCEL));

	d->CapacityOfVertices = 1;
	d->CapacityOfEdges = 2;	//Because they come in pairs
	d->CapacityOfFaces = 1;

	d->Vertices = calloc(d->CapacityOfVertices, sizeof(Vertex*));
	d->Edges = calloc(d->CapacityOfEdges, sizeof(HalfEdge*));
	d->Faces = calloc(d->CapacityOfFaces, sizeof(Face*));

	d->NumberOfVertices = 0;
	d->NumberOfEdges = 0;
	d->NumberOfFaces = 1;	//For the outer face

	for(int i = 0; i < d->NumberOfFaces; i++)
		d->Faces[i] = calloc(1, sizeof(Face));

	return d;
}

int getNumberOfFaces(DCEL *d) {
	return d->NumberOfFaces;
}

int getNumberOfEdges(DCEL *d) {
	return d->NumberOfEdges;
}

int getNumberOfVertices(DCEL *d) {
	return d->NumberOfVertices;
}

HalfEdge *nextIncidentEdge(Vertex *v, HalfEdge *e) {	//v is not needed here
	return e->PreviousEdge->TwinEdge;
}

Vertex *destination(HalfEdge *e) {
	return e->TwinEdge->Origin;
}

Vertex *makeVertex(DCEL *d, double x, double y) {
	if(d->NumberOfVertices == d->CapacityOfVertices) {
		d->CapacityOfVertices *= GROWTH_COEFFICIENT;
		d->Vertices = realloc(d->Vertices, d->CapacityOfVertices * sizeof(Vertex*));
		memset(&d->Vertices[d->NumberOfVertices + 1], 0, (d->NumberOfVertices - 1) * sizeof(Vertex*));
	}

	Vertex *v = d->Vertices[d->NumberOfVertices] = calloc(1, sizeof(Vertex));
	v->x = x;
	v->y = y;
	v->d = d;
	d->NumberOfVertices++;

	return v;
}

Face *nextFace(Vertex *v, Face *f) {
	HalfEdge *e = v->IncidentEdge;

	while(e->IncidentFace != f)
		e = nextIncidentEdge(v, e);

	return nextIncidentEdge(v, e)->IncidentFace;
}

int numberOfFaces(Vertex *v) {
	if(!v->IncidentEdge)
		return 1;

	int n = 0;
	Face *f = v->IncidentEdge->IncidentFace;

	do {
		n++;
		f = nextFace(v, f);
	} while(f != v->IncidentEdge->IncidentFace);

	return n;
}

Face *getOuterFace(DCEL *d) {
	return d->Faces[0];
}

Face *getCommonFace(Vertex *v1, Vertex *v2) {
	Face *outerFace = getOuterFace(v1->d);

	if(!v1->IncidentEdge || !v2->IncidentEdge)
		return outerFace;	//It is wrong to assume that but...

	int outer = 0;
	HalfEdge *e1 = v1->IncidentEdge;

	do {
		HalfEdge *e2 = v2->IncidentEdge;

		do {
			if(e1->IncidentFace == outerFace && e1->IncidentFace == e2->IncidentFace)
				outer = 1;
			else if(e1->IncidentFace == e2->IncidentFace)
				return e1->IncidentFace;

			e2 = nextIncidentEdge(v2, e2);
		} while(e2 != v2->IncidentEdge);

		e1 = nextIncidentEdge(v1, e1);
	} while(e1 != v1->IncidentEdge);

	if(outer)
		return outerFace;
	else
		return NULL;
}

int isAdjacent(Vertex *v1, Vertex *v2) {
	HalfEdge *e = v1->IncidentEdge;

	do {
		if(destination(e) == v2)
			return 1;
		e = nextIncidentEdge(v1, e);
	} while(e != v1->IncidentEdge);

	return 0;
}

HalfEdge *getIncidentEdge(Vertex *v, Face *f) {
	HalfEdge *e = v->IncidentEdge;

	do {
		if(e->IncidentFace == f)
			return e;
		e = nextIncidentEdge(v, e);
	} while(e != v->IncidentEdge);

	return NULL;
}

int numberOfEdges(Face *f) {
	if(!f->IncidentEdge)
		return 0;

	int n = 0;
	HalfEdge *e = f->IncidentEdge;

	do {
		n++;
		e = e->NextEdge;
	} while(e != f->IncidentEdge);

	return n;
}

void bindEdge(Face *f, Vertex *v, HalfEdge *e1, HalfEdge *e2) {
	if(!v->IncidentEdge) {
		v->IncidentEdge = e1;
		e1->PreviousEdge = e2;
		e2->NextEdge = e1;
	}
	else {
		HalfEdge *e = v->IncidentEdge;

		while(e->TwinEdge->IncidentFace != f)
			e = nextIncidentEdge(v, e);

		e->TwinEdge->NextEdge->PreviousEdge = e2;
		e2->NextEdge = e->TwinEdge->NextEdge;
		e->TwinEdge->NextEdge = e1;
		e1->PreviousEdge = e->TwinEdge;
	}
}

HalfEdge *makeEdge(DCEL *d, Face *f, Vertex *v1, Vertex *v2) {
	if(d->NumberOfEdges == d->CapacityOfEdges) {
		d->CapacityOfEdges *= GROWTH_COEFFICIENT;
		d->Edges = realloc(d->Edges, d->CapacityOfEdges * sizeof(HalfEdge*));
		memset(&d->Edges[d->NumberOfEdges + 2], 0, (d->NumberOfEdges - 2) * sizeof(HalfEdge*));
	}

	HalfEdge *e1 = d->Edges[d->NumberOfEdges] = calloc(1, sizeof(HalfEdge));
	HalfEdge *e2 = d->Edges[d->NumberOfEdges + 1] = calloc(1, sizeof(HalfEdge));

	e1->Origin = v1;
	e1->TwinEdge = e2;
	e1->IncidentFace = f;

	e2->Origin = v2;
	e2->TwinEdge = e1;
	e2->IncidentFace = f;

	bindEdge(f, v1, e1, e2);
	bindEdge(f, v2, e2, e1);

	if(!f->IncidentEdge)
		f->IncidentEdge = e1;

	d->NumberOfEdges += 2;

	return e1;
}

HalfEdge *splitFace(DCEL *d, Face *f, Vertex *v1, Vertex *v2) {
	if(d->NumberOfFaces == d->CapacityOfFaces) {
		d->CapacityOfFaces *= GROWTH_COEFFICIENT;
		d->Faces = realloc(d->Faces, d->CapacityOfFaces * sizeof(Face*));
		memset(&d->Faces[d->NumberOfFaces + 1], 0, (d->NumberOfFaces - 1) * sizeof(Face*));
	}

	HalfEdge *e1 = makeEdge(d, f, v1, v2);
	HalfEdge *e2 = e1->TwinEdge;

	Face *nf = d->Faces[d->NumberOfFaces] = calloc(1, sizeof(Face));
	nf->IncidentEdge = e2;

	do {
		e2->IncidentFace = nf;
		e2 = e2->NextEdge;
	} while(e2 != e1->TwinEdge);

	d->NumberOfFaces++;

	return e2;
}

int main(void) {
	DCEL *d = makeDCEL();
	
	Vertex *v1 = makeVertex(d, 0,0);
	Vertex *v2 = makeVertex(d, 1,0);
	Vertex *v3 = makeVertex(d, 1,1);
	Vertex *v4 = makeVertex(d, 0,1);
	assert(numberOfFaces(v1) == 1); // v1 has one face (the outer face)
	
	HalfEdge *e1 = makeEdge(d, getOuterFace(d), v1, v2);
	assert(numberOfFaces(v1) == 1);
	assert(numberOfFaces(v2) == 1);
	// v1 and v2 have one common face--the outer face
	assert(getCommonFace(v1, v2) != NULL);
	assert(getCommonFace(v1, v3) != NULL);
	// get the new edge using the getIncidentEdge function
	assert(getIncidentEdge(e1->Origin, e1->IncidentFace) == e1);
	
	makeEdge(d, getOuterFace(d), v2, v3);
	
	assert(isAdjacent(v1, v2));
	assert(!isAdjacent(v1, v3));
	
	// Create two new Edges to create the first bounded face
	makeEdge(d, getOuterFace(d), v3, v4);
	splitFace(d, getOuterFace(d), v4, v1);
	
	// All Vertices are adjacent to the two Faces
	assert(numberOfFaces(v1) == 2);
	assert(numberOfFaces(v2) == 2);
	assert(numberOfFaces(v3) == 2);
	assert(numberOfFaces(v4) == 2);

	// v1 and v2 have two common Faces, the bounded face and the outer face,
	// the bounded face should be returned
	assert(getCommonFace(v1, v2) != getOuterFace(d));

	assert(numberOfEdges(getCommonFace(v1, v2)) == 4);

	// Create a new edge that will split the bounded face, hence will result in two bounded Faces
	splitFace(d, getCommonFace(v1, v2), v4, v2);
	assert(getNumberOfFaces(d) == 3);
	
	assert(numberOfFaces(v4) == 3);

	return 0;
}
