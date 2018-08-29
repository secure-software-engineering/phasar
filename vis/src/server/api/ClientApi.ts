import { execFile } from 'child_process';
import * as express from 'express'
import { default as ExplodedSupergraph, ExplodedSupergraphModel } from '../dbSchema/ExplodedSupergraph';
import { default as Method, MethodModel } from '../dbSchema/Method'
import { default as Statement, StatementModel } from '../dbSchema/Statement'
import { default as DataflowFact, DataflowFactModel } from '../dbSchema/DataflowFact'
import { default as EdgeModel } from '../interfaces/EdgeModel'
import * as debug from 'debug';
import { FRAMEWORK_CWD } from '../config/config';
import { CyEdge, CyMethod, CyNode, CytoscapeGraph } from '../interfaces/CyModels'
const dc = debug('client_api');
const db = debug('process');
const de = debug('error');
const path = require('path');

/**
 * Starts execution of ide/ifds framework. Framework is executed in 
 * its own process. 
 * Request Format:
 * {filename, moduleOrProject, wpaOnOff, mem2regOnOff}
 */
export async function startProcess(req: express.Request, res: express.Response) {
    db('starts analysis for file %s', req.file.filename);

    const uploadPath = path.join(process.cwd(), "server/data/uploads/" + req.file.filename);
    try {
        let explodedSupergraph = new ExplodedSupergraph({ name: req.body.name });
        await explodedSupergraph.save();
        const graph_id: string = "" + explodedSupergraph._id;
        const analysis: string = req.body.analysis;
        //TODO: support multiple llvm files 
        const exec = execFile('build/phasar',
            ["-" + req.body.moduleProject, uploadPath, "--data-flow-analysis", analysis, "--wpa", Number(req.body.wpa), "--mode", "phasarLLVM", "-M", Number(req.body.mem2reg), "--printedgerec", 1, "--graph-id", graph_id]
            , { cwd: FRAMEWORK_CWD, maxBuffer: 1000000 * 1024, });

        exec.stdout.on('data', (data: string) => {
            db('%s', data);
        });

        exec.stderr.on('data', (data: any) => {
            db(`stderr: %s`, data);
        });

        exec.on('close', (code: number) => {
            db(`child process exited with code %d`, code);
            res.send({ "resultId": graph_id });
        });
    }
    catch (err) {
        de("Creation of new graph failed %s", err);
        res.sendStatus(400);
    }
}
export async function getAllMethods(req: express.Request, res: express.Response) {
    const graphID = req.params.graphId;
    let graph = await ExplodedSupergraph.findOne({ _id: graphID }).populate('cfg');
    let methods = graph.cfg as any as Array<MethodModel>;
    let result: { nodes: Array<CyNode>, edges: Array<CyEdge> } = { nodes: [], edges: [] };
    //  let cyGraph: CytoscapeGraph = { id: graph._id, method: cyMethod, methodName: methodNode.data.name }
    for (let i = 0; i < methods.length; i++) {
        let method = methods[i];
        console.log("get base method started for, ", method);
        let cyGraph = await getBaseMethod(graph, method._id, true);
        //console.log("cy result for this method, ", cyGraph);
        result.nodes = result.nodes.concat(cyGraph.method.nodes);
        result.edges = result.edges.concat(cyGraph.method.edges);
    }
    // console.log("all methods", result);
    res.send(result);
}
export async function getAllMethodIds(req: express.Request, res: express.Response) {
    const graphID = req.params.graphId;
    let graph = await ExplodedSupergraph.findOne({ _id: graphID }).populate('cfg');
    let methods = graph.cfg as any as Array<MethodModel>;
    let result: { [methodId: string]: string } = {};

    methods.forEach((method) => {
        result[method._id] = method.methodName;
    });

    res.send(result);
}

export async function searchInstructionString(req: express.Request, res: express.Response) {
    const graphID = req.params.graphId;
    const searchString = req.params.searchString;
    let graph = await ExplodedSupergraph.findOne({ _id: graphID });
    let nodes = await Statement.find({ graph: graph._id, content: { $regex: searchString } }, 'method methodName');

    let result: { [methodId: string]: string } = {};
    let methodIds = new Set<string>();
    let methodNames = new Set<string>();
    nodes.forEach((node) => {
        result[node.method.toString()] = node.methodName;

    });

    res.send(result);
}

export async function searchDfString(req: express.Request, res: express.Response) {
    const graphID = req.params.graphId;
    const searchString = req.params.searchString;
    let graph = await ExplodedSupergraph.findOne({ _id: graphID });
    let nodes = await DataflowFact.find({ graph: graph._id, content: { $regex: searchString } }, 'method methodName');

    let result: { [methodId: string]: string } = {};
    let methodIds = new Set<string>();
    let methodNames = new Set<string>();
    nodes.forEach((node) => {

        result[node.method.toString()] = node.methodName;

    });

    res.send(result);
}
/**
 * returns array of all graph ids in database.
 */
export async function getGraphIds(req: express.Request, res: express.Response) {
    try {
        dc("Getting all Graph ids");

        let graphs = await ExplodedSupergraph.find({});
        let graphArray = [];

        for (let graphI in graphs) {
            let graph = graphs[graphI];
            let graphJson = { _id: graph._id, name: graph.name, method_count: graph.cfg.length };
            graphArray.push(graphJson);
        }
        dc("Sending %d graph ids", graphArray.length);
        res.send(graphArray);
    }
    catch (err) {
        de("error while retreving all graph ids from database. %s", err);
        res.sendStatus(404);
    }
}

/**
 * returns analysis results for given resultId.
 * result format: 
 * { nodeNumber: Number, elements: Array }
 */
export async function getAnalysisResult(req: express.Request, res: express.Response) {
    const graphID = req.params.resultId;
    dc("Request for analysis results of graph %o", graphID);
    try {
        let graph = await ExplodedSupergraph.findOne({ _id: graphID });
        let elements = await getBaseMethod(graph);

        res.send(elements);
    }
    catch (err) {
        de("error getting analysis results for graph %o. Error: %s", graphID, err);
        res.sendStatus(404);
    }
}

export async function getDataflowFacts(req: express.Request, res: express.Response) {
    let dataflowFacts = await DataflowFact.find({ graph: req.params.graphId, statement: req.params.nodeId });
    let dfCompoundId = req.params.nodeId + "_flowFacts";
    let dataflowFactsCy: Array<CyNode> = [];
    dataflowFacts.forEach((dfFact) => {
        //add dataflow facts
        dataflowFactsCy.push({ data: { id: dfFact._id, name: dfFact.content, parent: dfCompoundId, type: 5 } });
    });
    res.send(dataflowFactsCy);
}

export async function newBaseMethod(req: express.Request, res: express.Response) {

    const graphID = req.params.graphId;
    const methodID = req.params.methodId;
    try {
        let graph = await ExplodedSupergraph.findOne({ _id: graphID });
        let elements = await getBaseMethod(graph, methodID);
        dc("elements of graph %o: %o", graphID, elements);
        res.send(elements);
    }
    catch (err) {
        de("error getting analysis results for graph %o. Error: %s", graphID, err);
        res.sendStatus(404);
    }
}
/**
 * @param {Object} graph
 *@param optional string with method id 
 * 
 * Gets all graph nodes and reconstruct graph from the 
 * stored information.
 */
async function getBaseMethod(graph: ExplodedSupergraphModel, method_id = '', allMethods = false): Promise<CytoscapeGraph> {
    try {
        let method: MethodModel;

        if (method_id != '') {
            method = await Method.findOne({ _id: method_id });
        }
        else {
            method = await Method.findOne({ graph: graph._id, methodName: 'main' });
        }

        //create cytoscapeGraph
        dc("found nodes for graph %o", graph._id, method);
        let cyMethod = await constructMethod(method, allMethods);
        dc("finished constructing method");
        let graphId: string = graph._id.toString();
        let methodNode: CyNode = { data: { id: method.methodName, parent: graphId, type: 4, name: method.methodName } };
        cyMethod.nodes.unshift(methodNode);
        let graphNode: CyNode = { data: { id: graphId, name: '' } };
        cyMethod.nodes.unshift(graphNode);
        let cyGraph: CytoscapeGraph = { id: graphId, method: cyMethod, methodName: methodNode.data.name }
        return cyGraph;
    }
    catch (err) {
        de("Praeprocessing Graph failed with error: %s", err);
    }
}

/**
 * Returns nodes of @param {Number} methodId in format 
 * { nodeNumber: Number, elements: Array }
 * 
 * Used to get on click information about a method which is
 * currently clusterd to one node. 
 */
export async function getMethod(req: express.Request, res: express.Response) {
    const methodId = req.params.methodId;
    const graphId = req.params.graphId;
    dc("Get nodes for method %o", methodId);
    try {
        let method = await Method.findOne({ graph: graphId, methodName: methodId });
        let elements = await constructMethod(method);

        res.send(elements);
    }
    catch (err) {
        de("Error getting content of method %o. Error: %s", methodId, err);
        res.sendStatus(404);
    }
}

/**
 * 
 * Reconstructs the information about 
 * the given @param {Array} nodes 
 * 
 * returns { nodeNumber: Number, elements: Array }
 */
async function constructMethod(method: MethodModel, allMethods = false): Promise<CyMethod> {
    dc("started constructing method");

    // Graph{
    //     Methods: {
    //         rows: {
    //             instructions,
    //             dataflowFacts: {
    //                 Facts
    //             }
    //         }
    //     }
    // }

    let cyMethod: CyMethod = { id: method.methodName, edges: [], nodes: [] };

    let callsites: Array<string> = method.callsites as any as Array<string>;
    let allCallsites = await Statement.find({ _id: { $in: callsites } });
    allCallsites.forEach(async (callsite) => {
        let targetMethods: Array<string> = callsite.targetMethods as any as Array<string>;
        let methods = await Method.find({ _id: { $in: targetMethods } });
        methods.forEach((tMethod) => {
            let targetMethod = {
                data: {
                    parent: method.methodName,
                    id: tMethod.methodName,
                    name: tMethod.methodName,
                    collapsed: allMethods ? 0 : 2,
                    type: 4
                }
            };
            cyMethod.nodes.push(targetMethod);
        });

    });
    let statements: Array<string> = method.statements as any as Array<string>;
    let allStatements = await Statement.find({ _id: { $in: statements } });
    allStatements.forEach(async (statement) => {
        let rowId = statement.stmtId + 'row';
        let rowCompound: CyNode = { data: { id: rowId, name: '', parent: method.methodName } };
        cyMethod.nodes.push(rowCompound);

        let cyStatement: CyNode = {
            data: {
                parent: rowId,
                id: statement.stmtId,
                name: statement.content,
                type: 1,
                method: method._id,
                methodName: method.methodName
            }
        };
        let dataFlowCompoundNode = {
            data: {
                id: statement.stmtId + "_flowFacts",
                parent: rowId,
                name: 'Dataflow Facts',
                collapsed: 2,
                type: 6,
                from: statement.stmtId
            }
        };
        cyMethod.nodes.push(cyStatement, dataFlowCompoundNode);
        statement.succs.forEach((succ) => {

            let edge: CyEdge = {
                data: {
                    id: statement.stmtId + "_" + succ,
                    source: statement.stmtId,
                    target: succ
                }
            };
            cyMethod.edges.push(edge);

        });
    });

    dc("finished node and edge linkage");

    return cyMethod;
}