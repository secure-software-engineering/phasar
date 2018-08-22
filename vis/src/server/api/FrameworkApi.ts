import { default as ExplodedSupergraph, ExplodedSupergraphModel } from '../dbSchema/ExplodedSupergraph'
import { default as Method, MethodModel } from '../dbSchema/Method'
import { default as DataflowFact, DataflowFactModel } from '../dbSchema/DataflowFact'
import { default as Statement, StatementModel } from '../dbSchema/Statement'
import * as dbApi from './DatabaseApi'
import * as fs from 'fs'
import * as express from 'express';
import * as debug from 'debug';
const df = debug('framework_api');
const de = debug('error');

/**
 * Adds node to graph 
 */
export async function addToGraph(req: express.Request, res: express.Response) {

    df("receiving file");
    let encoding = "utf8";
    let data = fs.readFileSync(req.file.path, encoding);
    parseGraph(data, res);

}

async function parseGraph(data: string, res: express.Response) {


    // {"methods", methods},
    // {"statements", statements},
    // {"dataflowFacts", dataflowfacts},
    // {"callsites", callsites},
    // {"returnsites", returnsites}
    let jsonData: any = JSON.parse(data);
    try {
        // console.log("received file content:", jsonData);
        const graph_id: string = jsonData["id"];
        let explodedSupergraph = await ExplodedSupergraph.findOne({ _id: graph_id });

        let methods = await insertMethods(jsonData["methods"], explodedSupergraph);
        let methodStatement = await insertStatements(jsonData["statements"], graph_id, methods);
        methods = methodStatement.methods as { [name: string]: MethodModel };
        let statements = methodStatement.statements as { [name: string]: StatementModel };

        statements = await insertDataflowFacts(jsonData["dataflowFacts"], statements, graph_id);
        res.sendStatus(200);
    }
    catch (error) {
        console.log("error occured", error);
        res.sendStatus(500);
    }
}

async function insertDataflowFacts(dataflowFacts: Array<any>, statements: { [name: string]: StatementModel }, graphId: string): Promise<{ [name: string]: StatementModel }> {

    for (let key in dataflowFacts) {
        let dfJson = dataflowFacts[key];
        let stmtId = dfJson["statementId"]
        let statement: StatementModel = statements[stmtId];
        let dataflowFact = new DataflowFact({ dfId: dfJson["id"], content: dfJson["content"], statement: statement.stmtId, methodName: statement.methodName, method: statement.method, graph: graphId });

        await dataflowFact.save();
        statement.dataflowFacts.push(dataflowFact._id);

    }
    for (let key in statements) {
        statements[key].save();
    }

    return statements;
}

async function insertStatements(statements: Array<any>, graphId: string, methods: { [name: string]: MethodModel }): Promise<{ [name: string]: { [name: string]: MethodModel | StatementModel } }> {
    let statementsDic: { [name: string]: StatementModel } = {};
    for (let key in statements) {
        let statementJson: any = statements[key];
        let methodName: string = statementJson["method"];
        let method = methods[methodName];
        let succs: Array<string> = statementJson["successors"];
        let type: number = statementJson["type"];

        let stmtId = statementJson["id"];
        let statement = new Statement({ type: type, graph: graphId, succs: succs, method: method, stmtId: stmtId, content: statementJson["content"], methodName: methodName });
        if (type == 1) {
            let tm: Array<string> = statementJson["targetMethods"];
            let tmM: Array<MethodModel> = [];
            tm.forEach(async (meth) => {
                statement.targetMethods.push(methods[meth]._id);
            });

            method.callsites.push(statement._id);
        }

        statementsDic[stmtId] = statement;
        method.statements.push(statement._id);
    }
    for (let key in methods) {
        methods[key].save();
    }
    return { "methods": methods, "statements": statementsDic };
}

async function insertMethods(methods: Array<any>, graph: ExplodedSupergraphModel): Promise<{ [name: string]: MethodModel }> {
    let methodDic: { [name: string]: MethodModel } = {};
    let methodIdArray = [];
    for (let key in methods) {
        let methodName: string = methods[key]["methodName"];
        let method = new Method({ methodName: methodName, graph: "" + graph._id });

        methodDic[methodName] = method;
        methodIdArray.push(method._id);
    }
    graph.cfg = methodIdArray;
    graph.save();
    return methodDic;
}