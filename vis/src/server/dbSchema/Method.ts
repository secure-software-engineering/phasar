import { Schema, Document, Model } from "mongoose";
import * as  dbApi from '../api/DatabaseApi';


export interface MethodModel extends Document {
    statements?: Array<{ type: Schema.Types.ObjectId, ref: string }>,
    callsites?: Array<{ type: Schema.Types.ObjectId, ref: string }>,
    methodName: string,
    graph: string,
    type?: string
}

const MethodSchema = new Schema(
    {
        statements: [{ type: Schema.Types.ObjectId, ref: 'Statement' }],
        callsites: [{ type: Schema.Types.ObjectId, ref: 'Statement' }],
        methodName: String,
        graph: String
    }
);

//Export model

const Method = dbApi.getDB().model<MethodModel>('Method', MethodSchema);
export default Method;