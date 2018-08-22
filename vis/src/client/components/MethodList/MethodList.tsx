import * as React from 'react';

interface State {
    methodIds: Array<string>
}

interface Props {
    methodMap: { [methodId: string]: string },
    selectMethod(methodId: string): void

}
export default class MethodList extends React.Component<Props, State> {

    constructor(props: Props) {
        super(props);
        this.state = {
            methodIds: []
        };

        for (let key in this.props.methodMap) {
            this.state.methodIds.push(key);
        }
        this.itemSelected = this.itemSelected.bind(this);
    }

    itemSelected(event: React.MouseEvent<HTMLElement>) {
        let methodId: string = event.currentTarget.id;
        this.props.selectMethod(methodId);

    }

    render() {
        return (
            <div>
                <ul style={{ width: '90%', maxHeight: '200px', overflow: 'hidden', overflowY: 'scroll' }}>
                    {this.state.methodIds.map((methodId) =>
                        <li id={methodId} key={methodId} onDoubleClick={this.itemSelected}>
                            {this.props.methodMap[methodId]}
                        </li>
                    )}
                </ul>
            </div>
        );
    }
}
