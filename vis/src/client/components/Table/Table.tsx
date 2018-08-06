import * as React from 'react'
import { Link, Route, Redirect } from 'react-router-dom'
import { get } from '../../utils/Ajax'
import { ExplodedSupergraph } from '../../interfaces/ExplodedSupergraph'

interface Props {
    data: Array<ExplodedSupergraph>
}
interface State {
    data: Array<ExplodedSupergraph>,
    selected: string,
    redirect: number
}

export default class Table extends React.Component<Props, State> {

    constructor(props: Props) {
        super(props);

        this.state = {
            data: props.data,
            selected: "",
            redirect: 0
        }
    }


    render() {
        var rows: Array<JSX.Element> = [];

        const handleClick = (e: React.MouseEvent<HTMLTableRowElement>) => {
            e.preventDefault();
            let selectedRow = e.currentTarget;
            const selectedId = selectedRow.getAttribute("data-id");
            this.setState({ selected: selectedId });
        }

        const showVisulization = () => {
            if (this.state.selected == "") {
                alert("Select a graph to visualize");
            } else {
                this.setState({ redirect: 1 });
            }
        };
        this.state.data.map((row) => {
            let cells: Array<JSX.Element> = [];
            cells.push(<td key={"idCell_" + row._id} id={"idCell_" + row._id} data-id={row._id}>{row.name}</td>);
            cells.push(<td key={"methodCount_" + row._id} id={"methodCount_" + row._id} data-id={row._id}>{row.method_count}</td>);
            rows.push(<tr key={row._id} id={row._id} onClick={handleClick} data-id={row._id} style={{ "backgroundColor": row._id == this.state.selected ? 'red' : 'white' }}>{cells}</tr>);
        });
        if (this.state.redirect) {

            return (<Redirect to={{
                pathname: '/Result',
                state: { "resultId": this.state.selected }
            }} />);
        }
        else {
            return (
                <div id="table_wrapper">
                    <button onClick={showVisulization}>Show Visualization</button>
                    <table>
                        <tbody>
                            <tr>
                                <th>Name</th>
                                <th>Method Count</th>
                            </tr>
                            {rows}
                        </tbody>
                    </table>
                </div>
            );
        }
    }
}
