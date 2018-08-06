import { Schema, Document, Model } from "mongoose";
import * as  dbApi from '../api/DatabaseApi';


export interface StatementModel extends Document {
    stmtId: string,
    method: { type: Schema.Types.ObjectId, ref: 'Method' },
    content: string,
    succs: Array<string>,
    targetMethods: [{ type: Schema.Types.ObjectId, ref: 'Method' }],
    dataflowFacts: [{ type: Schema.Types.ObjectId, ref: 'DataflowFact' }],
    graph: string,
    type: number,
    methodName: string
}

const StatementSchema = new Schema(
    {
        stmtId: String,
        type: Number,
        method: { type: Schema.Types.ObjectId, ref: 'Method' },
        content: String,
        targetMethods: [{ type: Schema.Types.ObjectId, ref: 'Method' }],
        dataflowFacts: [{ type: Schema.Types.ObjectId, ref: 'DataflowFact' }],
        graph: String,
        succs: [String],
        methodName: String
    }
);

//Export model

const Statement = dbApi.getDB().model<StatementModel>('Statement', StatementSchema);
export default Statement;