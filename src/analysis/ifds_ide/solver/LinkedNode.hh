/*
 * LinkedNode.hh
 *
 *  Created on: 23.11.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_LINKEDNODE_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_LINKEDNODE_HH_

/**
 * A data-flow fact that can be linked with other equal facts.
 * Equality and hash-code operations must <i>not</i> take the linking data structures into account!
 *
 * @deprecated Use {@link JoinHandlingNode} instead.
 */
template<class D>
class LinkedNode {
public:
	virtual ~LinkedNode() = default;
	/**
	 * Links this node to a neighbor node, i.e., to an abstraction that would have been merged
	 * with this one of paths were not being tracked.
	 */
	virtual void addNeighbor(D originalAbstraction) = 0;
	virtual void setCallingContext(D callingContext) = 0;
};


#endif /* ANALYSIS_IFDS_IDE_SOLVER_LINKEDNODE_HH_ */
