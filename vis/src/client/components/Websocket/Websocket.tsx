import * as React from 'react'
import { connect } from 'react-redux'
import { getWs, setWs } from '../../actions/ConfigActions'
import io from 'socket.io-client'

// @connect((store) => ({
//   ws: store.config.ws
// }))
interface Props{
 ws: SocketIOClient.Socket
}

export class Websocket extends React.Component<any, Props> {

  constructor(props: any) {
    super(props)
  }

  componentDidMount() {
    this.props.dispatch(setWs(io()))
  }

  componentWillReceiveProps(nextProps: Props) {
    var dispatch = this.props.dispatch
    if (nextProps.ws != {}) {
      var socket = nextProps.ws
    }
  }

  render() {
    return (<div />)
  }

}
