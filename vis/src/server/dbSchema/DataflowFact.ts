import { Schema, Document, Model } from "mongoose";
import * as  dbApi from '../api/DatabaseApi';

export interface DataflowFactModel extends Document {
    method: { type: Schema.Types.ObjectId, ref: string },
    methodName: string,
    graph: { type: Schema.Types.ObjectId, ref: string },
    statement: string,
    content: string,
    dfId: string
}


const DataflowFactSchema = new Schema(
    {
        dfId: String,
        method: { type: Schema.Types.ObjectId, ref: 'Method' },
        statement: String,
        methodName: String,
        graph: { type: Schema.Types.ObjectId, ref: 'ExplodedSupergraph' },
        content: String
    }
);

//Export model
const DataflowFact = dbApi.getDB().model<DataflowFactModel>('DataflowFact', DataflowFactSchema);
export default DataflowFact;