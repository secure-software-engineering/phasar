import { Schema, Document, Model } from "mongoose";
import * as  dbApi from '../api/DatabaseApi';


export interface ExplodedSupergraphModel extends Document {
    cfg: Array<{ type: Schema.Types.ObjectId, ref: string }>,
    name: string
}

const GraphSchema = new Schema(
    {
        cfg: [{ type: Schema.Types.ObjectId, ref: 'Method' }],
        name: String
    }
);

//Export model
const ExplodedSupergraph = dbApi.getDB().model<ExplodedSupergraphModel>('ExplodedSupergraph', GraphSchema);
export default ExplodedSupergraph;