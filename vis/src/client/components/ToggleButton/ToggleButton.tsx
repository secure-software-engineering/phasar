import * as React from 'react'
import { style, media, types, cssRaw } from 'typestyle'
import { percent, px, em } from 'csx';


interface Props {
  handleClick(toggle: boolean): void,
  isToggleOn: boolean,
  labelText?: string
}
interface State {
  toggle: boolean
}
export default class ToggleButton extends React.Component<Props, State> {
  constructor(props: Props) {
    super(props);
    this.state = {
      toggle: props.isToggleOn
    }
    // This binding is necessary to make `this` work in the callback
    this.handleClick = this.handleClick.bind(this);
  }

  handleClick() {
    this.props.handleClick(!this.props.isToggleOn)
  }

  getLabelStyle(): void {
    cssRaw(".switch {position: relative;\
       display: inline-block; \
       width: 60px;\
       height: 34px; }\
       \
       .switch input {display:none;}\
       \
       .slider {\
        position: absolute;\
        cursor: pointer;\
        top: 0;\
        left: 0;\
        right: 0;\
        bottom: 0;\
        background-color: #ccc;\
        -webkit-transition: .4s;\
        transition: .4s;\
      }\
      .slider:before {position: absolute;\
        content: \"\"; \
        height: 26px; \
        width: 26px; \
        left: 4px; \
        bottom: 4px; \
        background-color: white; \
        -webkit-transition: .4s; \
        transition: .4s; }\
        input:checked + .slider {\
        background-color: #2196F3;\
        }\
        \
        input:focus + .slider {\
          box-shadow: 0 0 1px #2196F3;\
        }\
        \
        input:checked + .slider:before {\
          -webkit-transform: translateX(26px);\
          -ms-transform: translateX(26px);\
          transform: translateX(26px);\
        }\
        \
        /* Rounded sliders */\
        .slider.round {\
          border-radius: 34px;\
        }\
        \
        .slider.round:before {\
          border-radius: 50%;\
        }\
");
  }

  render() {
    this.getLabelStyle();
    let myLabel = null
    if (this.props.labelText != undefined) {
      myLabel = <label>{this.props.labelText}</label>
    }
    return (
      <div>
        {myLabel}
        <label className="switch">
          <input id="test" checked={this.props.isToggleOn} onChange={this.handleClick.bind(this)} type="checkbox" />
          <span className="slider round"></span>
        </label>
      </div>
    );
  }
}