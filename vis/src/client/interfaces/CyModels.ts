export interface CyMethod {
    id: string,
    nodes: Array<CyNode>,
    edges: Array<CyEdge>,
    callsites: Array<string>,
    returnsites: Array<string>
}
export interface CyNode {
    data: { id: string, name?: string, from?: string, to?: string, type?: number, method?: string }
}
export interface CyEdge {
    data: { id: string, source?: string, target?: string }
}
export interface CytoscapeGraph {
    id: string,
    methods: Array<CyMethod>,
}