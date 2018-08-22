import * as io from 'socket.io-client'

var initialState = {
  ws: {}
}

export default function tweetReducer(state = initialState, action: any) {

  switch (action.type) {

    case "WS_CONFIG_FULFILLED": {
      var ws = io(`${action.payload.host}:${action.payload.port}`)
      return { ...state, ws: ws }
    }

    case "WS_SET": {
      return { ...state, ws: action.payload }
    }

  }
  return state
}
