#ifndef CONNECTEDCOMPONENT_H
#define CONNECTEDCOMPONENT_H

#include "knnGraph.h"

#include <vector>
#include <stack>

#include <vcg/complex/complex.h>
#include <vcg/complex/allocate.h>
#include <vcg/complex/algorithms/update/flag.h>


using namespace std;
using namespace vcg;

template <typename _MyMeshType, typename _MyVertexType>
class ComponentFinder {
public:
    static std::vector<_MyVertexType*> &FindComponent(_MyMeshType& m, _MyVertexType& v, int numOfNeighbours, int numOfHop);
    static std::vector<_MyVertexType*> &FindComponent(_MyMeshType& m, _MyVertexType& v, int numOfNeighbours, float dim, float maxHopDist, vector<_MyVertexType*> &borderVect, vector<_MyVertexType*> &notReachableVect);

    static void DeletePerVertexAttribute(_MyMeshType& m);

    static void Dijkstra(_MyMeshType& m, _MyVertexType& v, int numOfNeighbours, float maxHopDist, vector<_MyVertexType*> &notReachableVect);
};


/** This function returns a vector which stores pointers to the vertex of the connected component with
  * center v and max distance (in number of edges) 'numOfHop' from v.
  * It uses the knnGraph with k = numOfNeighbours to find the component.
  * In order to compute the connected component it is used a Depth-Limited search
  **/
template <typename _MyMeshType, typename _MyVertexType>
std::vector<_MyVertexType*> &ComponentFinder<_MyMeshType, _MyVertexType>::FindComponent(_MyMeshType& m, _MyVertexType& v, int numOfNeighbours, int numOfHop) {
    vector<_MyVertexType*> *resultVect = new vector<_MyVertexType*>();
    stack<_MyVertexType*> DFSStack;
    stack<int> levelStack;

    tri::UpdateFlags<_MyMeshType>::VertexClearV(m);

    KNNTree<_MyMeshType, _MyVertexType>::MakeKNNTree(m, numOfNeighbours);

    typename _MyMeshType::template PerVertexAttributeHandle<vector<_MyVertexType*>* > neighboursVect = vcg::tri::Allocator<_MyMeshType>::template GetPerVertexAttribute<vector<_MyVertexType*>* >(m,"KNNGraph");
    typename vector<_MyVertexType*>::iterator it;
    int actualLevel;

    DFSStack.push(&v);
    levelStack.push(0);
    v.SetV();
    resultVect->push_back(&v);


    // The Depth-Limited Search
    _MyVertexType* element;
    while (!DFSStack.empty()) {
        element = DFSStack.top();
        actualLevel = levelStack.top();
        DFSStack.pop();
        levelStack.pop();

        if (actualLevel < numOfHop) {
            for (it = neighboursVect[element]->begin(); it != neighboursVect[element]->end(); it++) {
                if (!((*it)->IsV())) {
                    DFSStack.push(*it);
                    levelStack.push(actualLevel + 1);
                    (*it)->SetV();
                    resultVect->push_back(*it);
                }
            }
        }
    }

    KNNTree<_MyMeshType, _MyVertexType>::DeleteKNNTree(m);

    return *resultVect;
}





template <typename _MyMeshType, typename _MyVertexType>
class Compare {
private:
    typename _MyMeshType::template PerVertexAttributeHandle<float> *distFromCenter;

public:
    Compare(typename _MyMeshType::template PerVertexAttributeHandle<float> *distFromCenter) {
        this->distFromCenter = distFromCenter;
    }

    bool operator() (const _MyVertexType* lhs, const _MyVertexType* rhs) const {
        return (*distFromCenter)[*lhs] > (*distFromCenter)[*rhs];
    }
};

/** This function returns a vector which stores pointers to the vertex of the connected component with
  * center v and max distance 'dim' from v.
  * It uses the knnGraph with k = numOfNeighbours to find the component.
  * In order to compute the connected component it is used a Shortest Path algorithm where maxHopDist is the
  * maximum distance we want between two vertex (with the Shortest Path we also compute the geodesic distance)
  **/
template <typename _MyMeshType, typename _MyVertexType>
std::vector<_MyVertexType*> &ComponentFinder<_MyMeshType, _MyVertexType>::FindComponent(_MyMeshType& m, _MyVertexType& v, int numOfNeighbours, float dim, float maxHopDist, vector<_MyVertexType*> &borderVect, vector<_MyVertexType*> &notReachableVect) {
    vector<_MyVertexType*> *resultVect = new vector<_MyVertexType*>();

    tri::UpdateFlags<_MyMeshType>::VertexClearV(m);

    bool hasDistParam = vcg::tri::HasPerVertexAttribute(m, "DistParam");

    typename _MyMeshType::template PerVertexAttributeHandle<float> distFromCenter;

    if (hasDistParam) distFromCenter = vcg::tri::Allocator<_MyMeshType>::template GetPerVertexAttribute<float>(m, std::string("DistParam"));
    else return *resultVect;

    for (typename _MyMeshType::VertexIterator vi = m.vert.begin(); vi != m.vert.end(); vi++) {
        if (distFromCenter[vi] < dim) resultVect->push_back(&*vi);
    }

    typename vector<_MyVertexType*>::iterator it;
    for (it = notReachableVect.begin(); it != notReachableVect.end(); it++) {
        if (distFromCenter[*it] < dim) borderVect.push_back(*it);
    }

    return *resultVect;
}


template <typename _MyMeshType, typename _MyVertexType>
void ComponentFinder<_MyMeshType, _MyVertexType>::Dijkstra(_MyMeshType& m, _MyVertexType& v, int numOfNeighbours, float maxHopDist, vector<_MyVertexType*> &notReachableVect) {
    bool hasKNNGraph = vcg::tri::HasPerVertexAttribute(m, "KNNGraph");
    bool hasDistParam = vcg::tri::HasPerVertexAttribute(m, "DistParam");

    notReachableVect.clear();

    typename _MyMeshType::template PerVertexAttributeHandle<float> distFromCenter;
    if (!hasDistParam) {
        distFromCenter = vcg::tri::Allocator<_MyMeshType>::template AddPerVertexAttribute<float>(m, std::string("DistParam"));
    }
    else distFromCenter = vcg::tri::Allocator<_MyMeshType>::template GetPerVertexAttribute<float>(m, std::string("DistParam"));

    if (!hasKNNGraph) {
        KNNTree<_MyMeshType, _MyVertexType>::MakeKNNTree(m, numOfNeighbours);
    }

    typename _MyMeshType::template PerVertexAttributeHandle<vector<_MyVertexType*>* > neighboursVect = vcg::tri::Allocator<_MyMeshType>::template GetPerVertexAttribute<vector<_MyVertexType*>* >(m,"KNNGraph");


    typename vector<_MyVertexType*>::iterator it;

    typedef priority_queue<_MyVertexType*, vector<_MyVertexType*>, Compare<_MyMeshType, _MyVertexType> > VertPriorityQueue;
    Compare<_MyMeshType, _MyVertexType> Comparator(&distFromCenter);
    VertPriorityQueue prQueue (Comparator);

    for (typename _MyMeshType::VertexIterator vi = m.vert.begin(); vi != m.vert.end(); vi++)
        distFromCenter[vi] = numeric_limits<float>::max();

    distFromCenter[v] = 0.f;

    tri::UpdateFlags<_MyMeshType>::VertexClearV(m);

    prQueue.push(&v);
    v.SetV();

    float distance;
    _MyVertexType* element;


    while (!prQueue.empty()) {
        element = prQueue.top();
        prQueue.pop();

        for (it = neighboursVect[element]->begin(); it != neighboursVect[element]->end(); it++) {
            distance = vcg::Distance((*it)->P(), element->P());

            if (distFromCenter[*element] + distance < distFromCenter[*it]) distFromCenter[*it] = distFromCenter[*element] + distance;
            if (!((*it)->IsV()) && distance <= maxHopDist) {
                prQueue.push(*it);
                (*it)->SetV();
            }
            else if (distance > maxHopDist) notReachableVect.push_back(*it);
        }
    }
}


template <typename _MyMeshType, typename _MyVertexType>
void ComponentFinder<_MyMeshType, _MyVertexType>::DeletePerVertexAttribute(_MyMeshType& m) {
    KNNTree<_MyMeshType, _MyVertexType>::DeleteKNNTree(m);

    bool hasDistParam = vcg::tri::HasPerVertexAttribute(m, "DistParam");
    if (hasDistParam) {
        vcg::tri::Allocator<_MyMeshType>::DeletePerVertexAttribute(m, "DistParam");
    }

    return;
}



#endif // CONNECTEDCOMPONENT_H