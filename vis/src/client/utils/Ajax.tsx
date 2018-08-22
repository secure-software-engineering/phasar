function json(response: Response): any {
  return response.json()
}

export function get(url: string) {
  return (
    fetch(new Request(url, {
      method: 'GET',
      credentials: 'same-origin',
      headers: new Headers({
        "Authorization": "Bearer " + localStorage.jwt
      })
    }))
      .then(json));
}

export function postFile(url: string, data: any) {
  let request: RequestInit = {
    method: 'POST',
    body: data
  };
  return fetch(url, request).then(json);
}

export function post(url: string, data: Object) {
  return (
    fetch(new Request(url, {
      method: 'POST',
      body: JSON.stringify(data),
      headers: new Headers({
        "Content-Type": "application/json",
        "Authorization": "Bearer " + localStorage.jwt
      }),
      credentials: 'same-origin'
    }))
      .then(json)
  );
}


