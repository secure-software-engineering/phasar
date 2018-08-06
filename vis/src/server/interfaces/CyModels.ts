export interface CyMethod {
    id: string,
    nodes: Array<CyNode>,
    edges: Array<CyEdge>,
}

export interface CyDataflowFacts {

}

export interface CyNode {
    data: {
        id: string,
        targetMethod?: string,
        name: string,
        from?: string,
        to?: string,
        type?: number,
        method?: string,
        methodName?: string,
        callsite?: string,
        returnsite?: string,
        dataflowFacts?: Array<CyNode>,
        parent?: string,
        collapsed?: number
    }
}
export interface CyEdge {
    data: { id: string, source: string, target: string }
}
export interface CytoscapeGraph {
    id: string,
    method: CyMethod,
    methodName: string
}