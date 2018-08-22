import { combineReducers } from "redux"

import defaultReducer from "./DefaultReducer"
import configReducer from "./ConfigReducer"


export default combineReducers({
  default: defaultReducer,
  config: configReducer,
})
