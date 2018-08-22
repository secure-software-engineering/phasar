export interface MyRoute {
    path: string,
    exact: boolean,
    header: () => JSX.Element,
    main: () => JSX.Element
}
