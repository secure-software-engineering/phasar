import { get } from '../utils/Ajax'

export function getWs() {
  return {
    type: "WS_CONFIG",
    payload: get("/api/config/ws")
  }
}

export function setWs(io: any) {
  return {
    type: "WS_SET",
    payload: io
  }
}
