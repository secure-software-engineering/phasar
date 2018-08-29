import * as React from 'react'
import { Link, Route } from 'react-router-dom'
import { get } from '../../utils/Ajax'
import Table from '../Table/Table'
import { ExplodedSupergraph } from '../../interfaces/ExplodedSupergraph'
import * as config from '../../config/config';

interface State {
    data: Array<ExplodedSupergraph>,
    loading: number
}

export default class Database extends React.Component<any, State> {

    constructor(props: any) {
        super(props);

        this.state = {
            data: [],
            loading: 1
        }

        get(config.GET_GRAPH_IDS).then((result: Array<ExplodedSupergraph>) => {
            this.setState({ data: result, loading: 0 });
        });

    }

    render() {
        if (this.state.loading) {
            return <div>loading</div>
        }
        else {
            return <Table data={this.state.data}></Table>
        }

    }
}
