#include <float.h>
#include <assert.h>
#include "meshEdit.h"
#include "mutablePriorityQueue.h"
#include "error_dialog.h"

namespace CMU462 {

	// find the previous HalfedgeIter of a given HalfedgeIter
	HalfedgeIter prev_Halfedge(HalfedgeIter x) {
		HalfedgeIter prev = x, curr = x->next();
		while (curr != x) {
			prev = curr;
			curr = curr->next();
		}
		return prev;
	}


	VertexIter HalfedgeMesh::splitEdge(EdgeIter e0) {
		// TODO: (meshEdit)
		// This method should split the given edge and return an iterator to the
		// newly inserted vertex. The halfedge of this vertex should point along
		// the edge that was split, rather than the new edges.
		HalfedgeIter h1 = e0->halfedge();
		HalfedgeIter h2 = h1->twin();
		FaceIter f1 = h1->face();
		FaceIter f2 = h2->face();

		if (f1->degree() > 3 || f2->degree() > 3) return h1->vertex();

		HalfedgeIter h1_next = h1->next();
		HalfedgeIter h1_prev = h1_next->next();
		HalfedgeIter h2_next = h2->next();
		HalfedgeIter h2_prev = h2_next->next();

		VertexIter v1 = h1->vertex();
		VertexIter v2 = h2->vertex();
		VertexIter v1_next = h1_prev->vertex();
		VertexIter v2_next = h2_prev->vertex();
		VertexIter new_vertex = newVertex();

		EdgeIter new_edge1 = newEdge();
		EdgeIter new_edge2 = newEdge();
		EdgeIter new_edge_down = newEdge();
		FaceIter new_f1 = newFace();
		FaceIter new_f2 = newFace();

		HalfedgeIter h1_left = newHalfedge();
		HalfedgeIter h1_right = newHalfedge();
		HalfedgeIter h2_left = newHalfedge();
		HalfedgeIter h2_right = newHalfedge();
		HalfedgeIter h_down_left = newHalfedge();
		HalfedgeIter h_down_right = newHalfedge();

		new_vertex->position = e0->centroid();
		new_vertex->halfedge() = h2;
		v1->halfedge() = h1;
		v2->halfedge() = h_down_right;

		new_edge1->isNew = true;
		new_edge2->isNew = true;
		new_edge_down->isNew = false;

		h1->next() = h1_left;
		h1_left->next() = h1_prev;
		h2_next->next() = h2_left;
		h2->vertex() = new_vertex;
		h2_left->next() = h2;
		h1_left->vertex() = new_vertex;
		h2_left->vertex() = v2_next;
		h1_left->face() = f1;
		h2_left->face() = f2;
		f1->halfedge() = h1;
		f2->halfedge() = h2;
		h1_left->edge() = new_edge1;
		h2_left->edge() = new_edge2;
		new_edge1->halfedge() = h1_left;
		new_edge2->halfedge() = h2_left;
		h1_left->twin() = h1_right;
		h1_right->twin() = h1_left;
		h2_left->twin() = h2_right;
		h2_right->twin() = h2_left;
		h1_right->edge() = new_edge1;
		h2_right->edge() = new_edge2;
		h1_right->vertex() = v1_next;
		h2_right->vertex() = new_vertex;
		h1_right->face() = new_f1;
		h2_right->face() = new_f2;
		new_f1->halfedge() = h1_right;
		new_f2->halfedge() = h2_right;
		h1_right->next() = h_down_left;
		h2_right->next() = h2_prev;
		h1_next->next() = h1_right;
		h1_next->face() = new_f1;
		h2_prev->next() = h_down_right;
		h2_prev->face() = new_f2;
		h_down_left->vertex() = new_vertex;
		h_down_right->vertex() = v2;
		h_down_left->twin() = h_down_right;
		h_down_right->twin() = h_down_left;
		h_down_left->edge() = new_edge_down;
		h_down_right->edge() = new_edge_down;
		new_edge_down->halfedge() = h_down_left;
		h_down_left->face() = new_f1;
		h_down_right->face() = new_f2;
		h_down_left->next() = h1_next;
		h_down_right->next() = h2_right;

		//showError("splitEdge() not implemented.");
		return new_vertex;
	}

VertexIter HalfedgeMesh::collapseEdge(EdgeIter e) {
	// TODO: (meshEdit)
	// This method should collapse the given edge and return an iterator to
	// the new vertex created by the collapse.

	HalfedgeIter temp;
	HalfedgeIter halfedge1 = e->halfedge();
	HalfedgeIter halfedge2 = e->halfedge()->twin();
	VertexIter vertex1 = halfedge1->vertex();
	VertexIter vertex2 = halfedge2->vertex();

	vector<HalfedgeIter> halfedge_to_change;
	temp = halfedge1;
	do {
		halfedge_to_change.push_back(temp);
		temp = temp->twin()->next();
	} while (temp != halfedge1);
	temp = halfedge2;
	do {
		halfedge_to_change.push_back(temp);
		temp = temp->twin()->next();
	} while (temp != halfedge2);

	Vector3D new_coord = (vertex1->position + vertex2->position) / 2.0;
	VertexIter new_vertex = newVertex();
	new_vertex->position = new_coord;

	for (auto x : halfedge_to_change) {
		x->vertex() = new_vertex;
	}
	new_vertex->halfedge() = halfedge_to_change[halfedge_to_change.size() - 1];

	for (auto x : vector<HalfedgeIter>{ halfedge1,halfedge2 }) {
		if (x->face()->degree() == 3) {
			HalfedgeIter a = x->next();
			HalfedgeIter b = a->next();
			a->twin()->twin() = b->twin();
			b->twin()->twin() = a->twin();
			b->twin()->edge() = a->edge();
			if (a->edge()->halfedge() == a)a->edge()->halfedge() = b->twin();
			if (b->vertex()->halfedge() == b) b->vertex()->halfedge() = a->twin();

			deleteFace(x->face());
			deleteEdge(b->edge());
			deleteHalfedge(a);
			deleteHalfedge(b);
		}
		else {
			if (x->face()->halfedge() == x) x->face()->halfedge() = x->next();
			prev_Halfedge(x)->next() = x->next();
		}
	}


	deleteHalfedge(halfedge1);
	deleteHalfedge(halfedge2);
	deleteVertex(vertex1);
	deleteVertex(vertex2);
	deleteEdge(e);
	//cout << vertex1->position[0] << " , "<< vertex1->position[1] << " , " << vertex1->position[2] << endl;
	//showError("HOYAAAAAAAAAAAAA");
	return new_vertex;
}

VertexIter HalfedgeMesh::collapseFace(FaceIter f) {
  // TODO: (meshEdit)
  // This method should collapse the given face and return an iterator to
  // the new vertex created by the collapse.
  showError("collapseFace() not implemented.");
  return VertexIter();
}

FaceIter HalfedgeMesh::eraseVertex(VertexIter v) {
  // TODO: (meshEdit)
  // This method should replace the given vertex and all its neighboring
  // edges and faces with a single face, returning the new face.

  return FaceIter();
}

FaceIter HalfedgeMesh::eraseEdge(EdgeIter e) {
  // TODO: (meshEdit)
  // This method should erase the given edge and return an iterator to the
  // merged face.

  showError("eraseVertex() not implemented.");
  return FaceIter();
}

EdgeIter HalfedgeMesh::flipEdge(EdgeIter e0) {
	// TODO: (meshEdit)
	// This method should flip the given edge and return an iterator to the
	// flipped edge.

	HalfedgeIter h1 = e0->halfedge();
	HalfedgeIter h2 = h1->twin();
	HalfedgeIter h1_next = h1->next();
	HalfedgeIter h1_next_next = h1->next()->next();
	HalfedgeIter h2_next = h2->next();
	HalfedgeIter h2_next_next = h2->next()->next();
	HalfedgeIter h1_prev = prev_Halfedge(h1);
	HalfedgeIter h2_prev = prev_Halfedge(h2);

	h1->vertex()->halfedge() = h2_next;
	h2->vertex()->halfedge() = h1_next;
	h1->face()->halfedge() = h1;
	h2->face()->halfedge() = h2;

	h1->vertex() = h2_next_next->vertex();
	h2->vertex() = h1_next_next->vertex();

	h1->next() = h1_next_next;
	h1_prev->next() = h2_next;
	h2_next->next() = h1;
	h2->next() = h2_next_next;
	h2_prev->next() = h1_next;
	h1_next->next() = h2;

	h1_next->face() = h2->face();
	h2_next->face() = h1->face();

	//showError("flipEdge() not implemented.");
	return e0;
}

void HalfedgeMesh::subdivideQuad(bool useCatmullClark) {
  // Unlike the local mesh operations (like bevel or edge flip), we will perform
  // subdivision by splitting *all* faces into quads "simultaneously."  Rather
  // than operating directly on the halfedge data structure (which as you've
  // seen
  // is quite difficult to maintain!) we are going to do something a bit nicer:
  //
  //    1. Create a raw list of vertex positions and faces (rather than a full-
  //       blown halfedge mesh).
  //
  //    2. Build a new halfedge mesh from these lists, replacing the old one.
  //
  // Sometimes rebuilding a data structure from scratch is simpler (and even
  // more
  // efficient) than incrementally modifying the existing one.  These steps are
  // detailed below.

  // TODO Step I: Compute the vertex positions for the subdivided mesh.  Here
  // we're
  // going to do something a little bit strange: since we will have one vertex
  // in
  // the subdivided mesh for each vertex, edge, and face in the original mesh,
  // we
  // can nicely store the new vertex *positions* as attributes on vertices,
  // edges,
  // and faces of the original mesh.  These positions can then be conveniently
  // copied into the new, subdivided mesh.
  // [See subroutines for actual "TODO"s]
  if (useCatmullClark) {
    computeCatmullClarkPositions();
  } else {
    computeLinearSubdivisionPositions();
  }

  // TODO Step II: Assign a unique index (starting at 0) to each vertex, edge,
  // and
  // face in the original mesh.  These indices will be the indices of the
  // vertices
  // in the new (subdivided mesh).  They do not have to be assigned in any
  // particular
  // order, so long as no index is shared by more than one mesh element, and the
  // total number of indices is equal to V+E+F, i.e., the total number of
  // vertices
  // plus edges plus faces in the original mesh.  Basically we just need a
  // one-to-one
  // mapping between original mesh elements and subdivided mesh vertices.
  // [See subroutine for actual "TODO"s]
  assignSubdivisionIndices();

  // TODO Step III: Build a list of quads in the new (subdivided) mesh, as
  // tuples of
  // the element indices defined above.  In other words, each new quad should be
  // of
  // the form (i,j,k,l), where i,j,k and l are four of the indices stored on our
  // original mesh elements.  Note that it is essential to get the orientation
  // right
  // here: (i,j,k,l) is not the same as (l,k,j,i).  Indices of new faces should
  // circulate in the same direction as old faces (think about the right-hand
  // rule).
  // [See subroutines for actual "TODO"s]
  vector<vector<Index> > subDFaces;
  vector<Vector3D> subDVertices;
  buildSubdivisionFaceList(subDFaces);
  buildSubdivisionVertexList(subDVertices);

  // TODO Step IV: Pass the list of vertices and quads to a routine that clears
  // the
  // internal data for this halfedge mesh, and builds new halfedge data from
  // scratch,
  // using the two lists.
  rebuild(subDFaces, subDVertices);
}

/**
 * Compute new vertex positions for a mesh that splits each polygon
 * into quads (by inserting a vertex at the face midpoint and each
 * of the edge midpoints).  The new vertex positions will be stored
 * in the members Vertex::newPosition, Edge::newPosition, and
 * Face::newPosition.  The values of the positions are based on
 * simple linear interpolation, e.g., the edge midpoints and face
 * centroids.
 */
void HalfedgeMesh::computeLinearSubdivisionPositions() {
	// TODO For each vertex, assign Vertex::newPosition to
	// its original position, Vertex::position.
	for (VertexIter x = verticesBegin(); x != verticesEnd(); x++) {
		x->newPosition = x->position;
	}

	// TODO For each edge, assign the midpoint of the two original
	// positions to Edge::newPosition.
	for (EdgeIter x = edgesBegin(); x != edgesEnd(); x++) {
		x->newPosition = x->centroid();
	}

	// TODO For each face, assign the centroid (i.e., arithmetic mean)
	// of the original vertex positions to Face::newPosition.  Note
	// that in general, NOT all faces will be triangles!
	for (FaceIter x = facesBegin(); x != facesEnd(); x++) {
		x->newPosition = x->centroid();
	}

	//showError("computeLinearSubdivisionPositions() not implemented.");
}

/**
 * Compute new vertex positions for a mesh that splits each polygon
 * into quads (by inserting a vertex at the face midpoint and each
 * of the edge midpoints).  The new vertex positions will be stored
 * in the members Vertex::newPosition, Edge::newPosition, and
 * Face::newPosition.  The values of the positions are based on
 * the Catmull-Clark rules for subdivision.
 */
void HalfedgeMesh::computeCatmullClarkPositions() {
	// TODO The implementation for this routine should be
	// a lot like HalfedgeMesh::computeLinearSubdivisionPositions(),
	// except that the calculation of the positions themsevles is
	// slightly more involved, using the Catmull-Clark subdivision
	// rules. (These rules are outlined in the Developer Manual.)

	// TODO face
	for (FaceIter x = facesBegin(); x != facesEnd(); x++) {
		x->newPosition = x->centroid();
	}

	// TODO edges
	for (EdgeIter x = edgesBegin(); x != edgesEnd(); x++) {
		HalfedgeIter h1 = x->halfedge();
		HalfedgeIter h2 = h1->twin();
		Vector3D a = h1->vertex()->position;
		Vector3D b = h2->vertex()->position;
		Vector3D c = h1->face()->newPosition;
		Vector3D d = h2->face()->newPosition;

		x->newPosition = (a + b + c + d) / 4.0;
	}

	// TODO vertices
	for (VertexIter x = verticesBegin(); x != verticesEnd(); x++) {
		int n = 0;
		HalfedgeIter h1 = x->halfedge();
		Vector3D Q = { 0.0,0.0,0.0 };
		Vector3D R = { 0.0,0.0,0.0 };
		Vector3D S = x->position;
		do {
			n++;
			Q += h1->face()->newPosition;
			R += h1->edge()->centroid();
			h1 = h1->twin()->next();
		} while (h1 != x->halfedge());
		Q /= n;
		R /= n;

		x->newPosition = (Q + 2 * R + (n - 3)*S) / n;
	}
	
	//showError("computeCatmullClarkPositions() not implemented.");
}

/**
 * Assign a unique integer index to each vertex, edge, and face in
 * the mesh, starting at 0 and incrementing by 1 for each element.
 * These indices will be used as the vertex indices for a mesh
 * subdivided using Catmull-Clark (or linear) subdivision.
 */
void HalfedgeMesh::assignSubdivisionIndices() {
	// TODO Start a counter at zero; if you like, you can use the
	// "Index" type (defined in halfedgeMesh.h)
	Index i = 0;

	// TODO Iterate over vertices, assigning values to Vertex::index
	for (VertexIter x = verticesBegin(); x != verticesEnd(); x++) {
		x->index = i;
		i++;
	}

	// TODO Iterate over edges, assigning values to Edge::index
	for (EdgeIter x = edgesBegin(); x != edgesEnd(); x++) {
		x->index = i;
		i++;
	}

	// TODO Iterate over faces, assigning values to Face::index
	for (FaceIter x = facesBegin(); x != facesEnd(); x++) {
		x->index = i;
		i++;
	}

	//showError("assignSubdivisionIndices() not implemented.");
}

/**
 * Build a flat list containing all the vertex positions for a
 * Catmull-Clark (or linear) subdivison of this mesh.  The order of
 * vertex positions in this list must be identical to the order
 * of indices assigned to Vertex::newPosition, Edge::newPosition,
 * and Face::newPosition.
 */
void HalfedgeMesh::buildSubdivisionVertexList(vector<Vector3D>& subDVertices) {
	// TODO Resize the vertex list so that it can hold all the vertices.
	Size size = nEdges() + nFaces() + nVertices();
	subDVertices.resize(size);

	// TODO Iterate over vertices, assigning Vertex::newPosition to the
	// appropriate location in the new vertex list.
	for (VertexIter x = verticesBegin(); x != verticesEnd(); x++) {
		subDVertices[x->index] = x->newPosition;
	} 

	// TODO Iterate over edges, assigning Edge::newPosition to the appropriate
	// location in the new vertex list.
	for (EdgeIter x = edgesBegin(); x != edgesEnd(); x++) {
		subDVertices[x->index] = x->newPosition;
	}

	// TODO Iterate over faces, assigning Face::newPosition to the appropriate
	// location in the new vertex list.
	for (FaceIter x = facesBegin(); x != facesEnd(); x++) {
		subDVertices[x->index] = x->newPosition;
	}

	//showError("buildSubdivisionVertexList() not implemented.");
}

/**
 * Build a flat list containing all the quads in a Catmull-Clark
 * (or linear) subdivision of this mesh.  Each quad is specified
 * by a vector of four indices (i,j,k,l), which come from the
 * members Vertex::index, Edge::index, and Face::index.  Note that
 * the ordering of these indices is important because it determines
 * the orientation of the new quads; it is also important to avoid
 * "bowties."  For instance, (l,k,j,i) has the opposite orientation
 * of (i,j,k,l), and if (i,j,k,l) is a proper quad, then (i,k,j,l)
 * will look like a bowtie.
 */
void HalfedgeMesh::buildSubdivisionFaceList(vector<vector<Index> >& subDFaces) {
	// TODO This routine is perhaps the most tricky step in the construction of
	// a subdivision mesh (second, perhaps, to computing the actual Catmull-Clark
	// vertex positions).  Basically what you want to do is iterate over faces,
	// then for each for each face, append N quads to the list (where N is the
	// degree of the face).  For this routine, it may be more convenient to simply
	// append quads to the end of the list (rather than allocating it ahead of
	// time), though YMMV.  You can of course iterate around a face by starting
	// with its first halfedge and following the "next" pointer until you get
	// back to the beginning.  The tricky part is making sure you grab the right
	// indices in the right order---remember that there are indices on vertices,
	// edges, AND faces of the original mesh.  All of these should get used.  Also
	// remember that you must have FOUR indices per face, since you are making a
	// QUAD mesh!

	// TODO iterate over faces
	// TODO loop around face
	// TODO build lists of four indices for each sub-quad
	// TODO append each list of four indices to face list
	for (FaceIter f = facesBegin(); f != facesEnd(); f++) {
		HalfedgeIter it = f->halfedge();
		HalfedgeIter it2 = it->next();
		do {
			subDFaces.push_back({ it2->vertex()->index,it2->edge()->index, f->index,it->edge()->index });
			it = it->next();
			it2 = it->next();
		} while (it != f->halfedge());
	}

	//showError("buildSubdivisionFaceList() not implemented.");
}

FaceIter HalfedgeMesh::bevelVertex(VertexIter v) {
  // TODO This method should replace the vertex v with a face, corresponding to
  // a bevel operation. It should return the new face.  NOTE: This method is
  // responsible for updating the *connectivity* of the mesh only---it does not
  // need to update the vertex positions.  These positions will be updated in
  // HalfedgeMesh::bevelVertexComputeNewPositions (which you also have to
  // implement!)

  showError("bevelVertex() not implemented.");
  return facesBegin();
}

FaceIter HalfedgeMesh::bevelEdge(EdgeIter e) {
  // TODO This method should replace the edge e with a face, corresponding to a
  // bevel operation. It should return the new face.  NOTE: This method is
  // responsible for updating the *connectivity* of the mesh only---it does not
  // need to update the vertex positions.  These positions will be updated in
  // HalfedgeMesh::bevelEdgeComputeNewPositions (which you also have to
  // implement!)

  showError("bevelEdge() not implemented.");
  return facesBegin();
}

FaceIter HalfedgeMesh::bevelFace(FaceIter f) {
	// TODO This method should replace the face f with an additional, inset face
	// (and ring of faces around it), corresponding to a bevel operation. It
	// should return the new face.  NOTE: This method is responsible for updating
	// the *connectivity* of the mesh only---it does not need to update the vertex
	// positions.  These positions will be updated in
	// HalfedgeMesh::bevelFaceComputeNewPositions (which you also have to
	// implement!)

	int n = f->degree();
	vector<VertexIter> old_vertex;
	vector<HalfedgeIter> old_halfedge;
	HalfedgeIter h = f->halfedge();
	do {
		old_vertex.push_back(h->vertex());
		old_halfedge.push_back(h);
		h = h->next();
	} while (h != f->halfedge());

	FaceIter new_face = f;
	vector<FaceIter> sideFace;
	vector<EdgeIter> innerEdge;
	vector<EdgeIter> connectingEdge;
	vector<VertexIter> new_vertex;
	vector<HalfedgeIter> outward_halfedge;
	vector<HalfedgeIter> inward_halfedge;
	vector<HalfedgeIter> inside_halfedge;
	vector<HalfedgeIter> inside_twin_halfedge;

	for (int i = 0; i < n; i++) {
		sideFace.push_back(newFace());
		innerEdge.push_back(newEdge());
		connectingEdge.push_back(newEdge());
		new_vertex.push_back(newVertex());
		outward_halfedge.push_back(newHalfedge());
		inward_halfedge.push_back(newHalfedge());
		inside_halfedge.push_back(newHalfedge());
		inside_twin_halfedge.push_back(newHalfedge());
	}

	int i_plus_1, i_minus_1;
	for (int i = 0; i < n; i++) {
		i_plus_1 = (i + 1) % n;
		i_minus_1 = (i - 1 + n) % n;

		new_vertex[i]->position = old_vertex[i]->position;

		old_vertex[i]->halfedge() = inward_halfedge[i];
		inward_halfedge[i]->vertex() = old_vertex[i];
		new_vertex[i]->halfedge() = outward_halfedge[i];
		outward_halfedge[i]->vertex() = new_vertex[i];
		inside_twin_halfedge[i]->vertex() = new_vertex[i_plus_1];
		inside_halfedge[i]->vertex() = new_vertex[i];

		outward_halfedge[i]->twin() = inward_halfedge[i];
		inward_halfedge[i]->twin() = outward_halfedge[i];
		inside_halfedge[i]->twin() = inside_twin_halfedge[i];
		inside_twin_halfedge[i]->twin() = inside_halfedge[i];
		
		outward_halfedge[i]->face() = sideFace[i];
		inward_halfedge[i]->face() = sideFace[i_minus_1];
		inside_twin_halfedge[i]->face() = sideFace[i];
		old_halfedge[i]->face() = sideFace[i];
		sideFace[i]->halfedge() = old_halfedge[i];
		inside_halfedge[i]->face() = new_face;
		
		inside_halfedge[i]->edge() = innerEdge[i];
		inside_twin_halfedge[i]->edge() = innerEdge[i];
		innerEdge[i]->halfedge() = inside_halfedge[i];

		outward_halfedge[i]->edge() = connectingEdge[i];
		inward_halfedge[i]->edge() = connectingEdge[i];
		connectingEdge[i]->halfedge() = inward_halfedge[i];

		old_halfedge[i]->next() = inward_halfedge[i_plus_1];
		inward_halfedge[i]->next() = inside_twin_halfedge[i_minus_1];
		inside_twin_halfedge[i]->next() = outward_halfedge[i];
		outward_halfedge[i]->next() = old_halfedge[i];
		inside_halfedge[i]->next() = inside_halfedge[i_plus_1];
	}
	new_face->halfedge() = inside_halfedge[0];
	//showError("bevelFace() not implemented.");
	return new_face;
}


void HalfedgeMesh::bevelFaceComputeNewPositions(
	vector<Vector3D>& originalVertexPositions,
	vector<HalfedgeIter>& newHalfedges, double normalShift,
	double tangentialInset) {
	// TODO Compute new vertex positions for the vertices of the beveled face.
	//
	// These vertices can be accessed via newHalfedges[i]->vertex()->position for
	// i = 1, ..., newHalfedges.size()-1.
	//
	// The basic strategy here is to loop over the list of outgoing halfedges,
	// and use the preceding and next vertex position from the original mesh
	// (in the originalVertexPositions array) to compute an offset vertex
	// position.
	//
	// Note that there is a 1-to-1 correspondence between halfedges in
	// newHalfedges and vertex positions
	// in orig.  So, you can write loops of the form
	//
	// for( int i = 0; i < newHalfedges.size(); i++ )
	// {
	//    Vector3D pi = originalVertexPositions[i]; // get the original vertex
	//    position correponding to vertex i
	// }
	//
	int i_plus_1,i_minus_1,n = newHalfedges.size();
	for (int i = 0; i < n; i++){
		i_plus_1 = (i + 1) % n;
		i_minus_1 = (i - 1 + n) % n;

		Vector3D a = originalVertexPositions[i];
		Vector3D b = originalVertexPositions[i_plus_1];
		Vector3D c = originalVertexPositions[i_minus_1];

		Vector3D ab = b - a;
		//ab /= ab.norm();
		ab = ab.unit();
		Vector3D ac = c - a;
		ac = ac.unit();

		// tangential shift
		Vector3D dir = ab + ac;
		dir = dir.unit();
		Vector3D perpendicular = dir - ab * dot(dir,ab);
		double distance = tangentialInset / perpendicular.norm();
		newHalfedges[i]->vertex()->position += dir * distance;

		// normal shift
		Vector3D normal = cross(ab, ac);
		normal = normal.unit();
		newHalfedges[i]->vertex()->position += normal * normalShift;
	}
}

void HalfedgeMesh::bevelVertexComputeNewPositions(
    Vector3D originalVertexPosition, vector<HalfedgeIter>& newHalfedges,
    double tangentialInset) {
  // TODO Compute new vertex positions for the vertices of the beveled vertex.
  //
  // These vertices can be accessed via newHalfedges[i]->vertex()->position for
  // i = 1, ..., hs.size()-1.
  //
  // The basic strategy here is to loop over the list of outgoing halfedges,
  // and use the preceding and next vertex position from the original mesh
  // (in the orig array) to compute an offset vertex position.

}

void HalfedgeMesh::bevelEdgeComputeNewPositions(
    vector<Vector3D>& originalVertexPositions,
    vector<HalfedgeIter>& newHalfedges, double tangentialInset) {
  // TODO Compute new vertex positions for the vertices of the beveled edge.
  //
  // These vertices can be accessed via newHalfedges[i]->vertex()->position for
  // i = 1, ..., newHalfedges.size()-1.
  //
  // The basic strategy here is to loop over the list of outgoing halfedges,
  // and use the preceding and next vertex position from the original mesh
  // (in the orig array) to compute an offset vertex position.
  //
  // Note that there is a 1-to-1 correspondence between halfedges in
  // newHalfedges and vertex positions
  // in orig.  So, you can write loops of the form
  //
  // for( int i = 0; i < newHalfedges.size(); i++ )
  // {
  //    Vector3D pi = originalVertexPositions[i]; // get the original vertex
  //    position correponding to vertex i
  // }
  //

}

void HalfedgeMesh::splitPolygons(vector<FaceIter>& fcs) {
  for (auto f : fcs) splitPolygon(f);
}

void HalfedgeMesh::splitPolygon(FaceIter f) {
	// TODO: (meshedit) 
	// Triangulate a polygonal face
	if (f->degree() < 4) return;

	VertexIter main_vertex = f->halfedge()->vertex();
	HalfedgeIter last_Halfedge = prev_Halfedge(f->halfedge());
	vector<VertexIter> vertexes;

	HalfedgeIter temp = f->halfedge()->next()->next();
	do {
		vertexes.push_back(temp->vertex());
		temp = temp->next();
	} while (temp != f->halfedge());
	vertexes.pop_back();


	for (VertexIter curr_vertex : vertexes) {
		FaceIter new_Face = newFace();
		EdgeIter new_Edge = newEdge();
		HalfedgeIter new_Halfedge1 = newHalfedge();
		HalfedgeIter new_Halfedge2 = newHalfedge();

		HalfedgeIter main_Halfedge = f->halfedge();

		new_Halfedge1->edge() = new_Edge;
		new_Halfedge2->edge() = new_Edge;
		new_Edge->halfedge() = new_Halfedge1;

		new_Halfedge1->twin() = new_Halfedge2;
		new_Halfedge2->twin() = new_Halfedge1;

		new_Halfedge1->vertex() = curr_vertex;
		curr_vertex->halfedge() = new_Halfedge1;
		new_Halfedge2->vertex() = main_vertex;
		main_vertex->halfedge() = new_Halfedge2;

		new_Halfedge1->face() = new_Face;
		new_Face->halfedge() = new_Halfedge1;
		new_Halfedge2->face() = f;
		f->halfedge() = new_Halfedge2;

		new_Halfedge2->next() = main_Halfedge->next()->next();
		main_Halfedge->next()->next() = new_Halfedge1;
		new_Halfedge1->next() = main_Halfedge;
		last_Halfedge->next() = new_Halfedge2;

		main_Halfedge->face() = new_Face;
		main_Halfedge->next()->face() = new_Face;
	}

	//showError("splitPolygon() not implemented.");
}

EdgeRecord::EdgeRecord(EdgeIter& _edge) : edge(_edge) {
	// TODO: (meshEdit)
	// Compute the combined quadric from the edge endpoints.
	Matrix4x4 q = edge->halfedge()->vertex()->quadric;
	q += edge->halfedge()->twin()->vertex()->quadric;

	// -> Build the 3x3 linear system whose solution minimizes the quadric error
	//    associated with these two endpoints.
	Matrix3x3 A ;
	A(0, 0) = q(0, 0);
	A(0, 1) = q(0, 1);
	A(0, 2) = q(0, 2);
	A(1, 0) = q(1, 0);
	A(1, 1) = q(1, 1);
	A(1, 2) = q(1, 2);
	A(2, 0) = q(2, 0);
	A(2, 1) = q(2, 1);
	A(2, 2) = q(2, 2);
	Vector3D b (-q(0,3), -q(1, 3), -q(2, 3));

	// -> Use this system to solve for the optimal position, and store it in
	//    EdgeRecord::optimalPoint.
	optimalPoint = A.inv()*b;

	// -> Also store the cost associated with collapsing this edg in
	//    EdgeRecord::Cost
	Vector4D x(optimalPoint, 1);
	score = dot(x , q*x);
}

void MeshResampler::upsample(HalfedgeMesh& mesh)
// This routine should increase the number of triangles in the mesh using Loop
// subdivision.
{
	// TODO: (meshEdit)
	// Compute new positions for all the vertices in the input mesh, using
	// the Loop subdivision rule, and store them in Vertex::newPosition.
	// -> At this point, we also want to mark each vertex as being a vertex of the
	//    original mesh.
	// -> Next, compute the updated vertex positions associated with edges, and
	//    store it in Edge::newPosition.
	// -> Next, we're going to split every edge in the mesh, in any order.  For
	//    future reference, we're also going to store some information about which
	//    subdivided edges come from splitting an edge in the original mesh, and
	//    which edges are new, by setting the flat Edge::isNew. Note that in this
	//    loop, we only want to iterate over edges of the original mesh.
	//    Otherwise, we'll end up splitting edges that we just split (and the
	//    loop will never end!)
	// -> Now flip any new edge that connects an old and new vertex.
	// -> Finally, copy the new vertex positions into final Vertex::position.
	
	// Each vertex and edge of the original surface can be associated with a
	// vertex in the new (subdivided) surface.
	// Therefore, our strategy for computing the subdivided vertex locations is to
	// *first* compute the new positions
	// using the connectity of the original (coarse) mesh; navigating this mesh
	// will be much easier than navigating
	// the new subdivided (fine) mesh, which has more elements to traverse.  We
	// will then assign vertex positions in
	// the new mesh based on the values we computed for the original mesh.
	for (VertexIter x = mesh.verticesBegin(); x != mesh.verticesEnd(); x++) {
		x->isNew = false;
	}
	for (EdgeIter x = mesh.edgesBegin(); x != mesh.edgesEnd(); x++) {
		x->isNew = false;
	}

	// Compute updated positions for all the vertices in the original mesh, using
	// the Loop subdivision rule.
	int n;
	double u;
	HalfedgeIter h;
	for (VertexIter x = mesh.verticesBegin(); x != mesh.verticesEnd(); x++) {
		Vector3D sum = { 0.0,0.0,0.0 };
		n = 0;
		h = x->halfedge();
		do {
			n++;
			sum += h->twin()->vertex()->position;
			h = h->twin()->next();
		} while (h != x->halfedge());
		
		if (n == 3) u = 3.0 / 16.0;
		else u = 3.0 / 8.0 / n;

		x->newPosition = (1.0-n*u)*x->position + u*sum;
	}

	// Next, compute the updated vertex positions associated with edges.
	for (EdgeIter x = mesh.edgesBegin(); x != mesh.edgesEnd(); x++) {
		Vector3D A = x->halfedge()->vertex()->position;
		Vector3D B = x->halfedge()->twin()->vertex()->position;
		Vector3D C = x->halfedge()->next()->next()->vertex()->position;
		Vector3D D = x->halfedge()->twin()->next()->next()->vertex()->position;

		x->newPosition = 3.0 / 8.0*(A + B) + 1.0 / 8.0*(C + D);
	}

	// Next, we're going to split every edge in the mesh, in any order.  For
	// future
	// reference, we're also going to store some information about which
	// subdivided
	// edges come from splitting an edge in the original mesh, and which edges are
	// new.
	// In this loop, we only want to iterate over edges of the original
	// mesh---otherwise,
	// we'll end up splitting edges that we just split (and the loop will never
	// end!)
	int N_edges = mesh.nEdges();
	VertexIter new_v;
	EdgeIter e = mesh.edgesBegin();
	EdgeIter e_copy = e;
	for (int i = 0; i < N_edges; i++) {
		e_copy++;
		new_v = mesh.splitEdge(e);
		new_v->isNew = true;
		e = e_copy;
	}

	// Finally, flip any new edge that connects an old and new vertex.
	for (EdgeIter x = mesh.edgesBegin(); x != mesh.edgesEnd(); x++) {
		if (x->isNew) {
			bool a = x->halfedge()->vertex()->isNew;
			bool b = x->halfedge()->twin()->vertex()->isNew;
			if (a != b) mesh.flipEdge(x);
		}
	}

	// Copy the updated vertex positions to the subdivided mesh.
	for (VertexIter x = mesh.verticesBegin(); x != mesh.verticesEnd(); x++) {
		if (!x->isNew) x->position = x->newPosition;
	}
	e = mesh.edgesBegin();
	for (int i = 0; i < N_edges; i++) {
		if (e->halfedge()->vertex()->isNew) {
			e->halfedge()->vertex()->position = e->newPosition;
		}
		else e->halfedge()->twin()->vertex()->position = e->newPosition;
		e++;
	}

	//showError("upsample() not implemented.");
}

void MeshResampler::downsample(HalfedgeMesh& mesh) {
	// TODO: (meshEdit)
	// Compute initial quadrics for each face by simply writing the plane equation
	// for the face in homogeneous coordinates. These quadrics should be stored
	// in Face::quadric
	for (FaceIter f = mesh.facesBegin(); f != mesh.facesEnd(); f++) {
		Vector3D N = f->normal(); // (a,b,c)
		N = N.unit();
		Vector3D p = f->halfedge()->vertex()->position;
		double d = -dot(N, p);
		Vector4D v(N, d); // (a,b,c,d)
		f->quadric = outer(v, v);
	}

	// -> Compute an initial quadric for each vertex as the sum of the quadrics
	//    associated with the incident faces, storing it in Vertex::quadric
	for (VertexIter v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++) {
		HalfedgeIter h = v->halfedge();
		v->quadric.zero();
		do {
			v->quadric += h->face()->quadric;
			h = h->twin()->next();
		} while (h != v->halfedge());
	}

	// -> Build a priority queue of edges according to their quadric error cost,
	//    i.e., by building an EdgeRecord for each edge and sticking it in the
	//    queue.
	MutablePriorityQueue<EdgeRecord> queue;
	for (EdgeIter e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
		e->record = EdgeRecord(e);
		queue.insert(e->record);
	}

	// -> Until we reach the target edge budget, collapse the best edge. Remember
	//    to remove from the queue any edge that touches the collapsing edge
	//    BEFORE it gets collapsed, and add back into the queue any edge touching
	//    the collapsed vertex AFTER it's been collapsed. Also remember to assign
	//    a quadric to the collapsed vertex, and to pop the collapsed edge off the
	//    top of the queue.
	int target = mesh.nEdges() / 4;
	while(mesh.nEdges() > target){
		EdgeRecord er = queue.top();
		queue.pop();
		EdgeIter temp, e = er.edge;

		// Compute the combined quadric from the edge endpoints.
		VertexIter v1 = e->halfedge()->vertex();
		VertexIter v2 = e->halfedge()->twin()->vertex();
		Matrix4x4 q = v1->quadric;
		q += v2->quadric;

		// remove from the queue any edge that touches the collapsing edge
		// BEFORE it gets collapsed
		HalfedgeIter h = v1->halfedge();
		do {
			temp = h->edge();
			if (temp != e) queue.remove(temp->record);
			h = h->twin()->next();
		}while (h != v1->halfedge());
		
		h = v2->halfedge();
		do {
			temp = h->edge();
			if (temp != e) queue.remove(temp->record);
			h = h->twin()->next();
		} while (h != v2->halfedge());

		// assign a quadric to the collapsed vertex
		VertexIter new_v = mesh.collapseEdge(e);
		new_v->position = er.optimalPoint;
		new_v->quadric = q;

		// add back into the queue any edge touching
		// the collapsed vertex AFTER it's been collapsed.
		h = new_v->halfedge();
		do {
			temp = h->edge();
			temp->record = EdgeRecord(temp);
			queue.insert(temp->record);
			h = h->twin()->next();
		} while (h != new_v->halfedge());
	}
	//showError("downsample() not implemented.");
}

void MeshResampler::resample(HalfedgeMesh& mesh) {
  // TODO: (meshEdit)
  // Compute the mean edge length.
  // Repeat the four main steps for 5 or 6 iterations
  // -> Split edges much longer than the target length (being careful about
  //    how the loop is written!)
  // -> Collapse edges much shorter than the target length.  Here we need to
  //    be EXTRA careful about advancing the loop, because many edges may have
  //    been destroyed by a collapse (which ones?)
  // -> Now flip each edge if it improves vertex degree
  // -> Finally, apply some tangential smoothing to the vertex positions
  showError("resample() not implemented.");
}

}  // namespace CMU462
