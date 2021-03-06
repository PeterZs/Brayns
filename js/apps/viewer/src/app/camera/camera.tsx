import React, {
    createRef,
    PureComponent,
    RefObject
} from 'react';

import {
    GET_CAMERA,
    INSPECT,
    InspectParams,
    SET_CAMERA
} from 'brayns';
import {isNumber} from 'lodash';
import {BehaviorSubject, fromEvent, Subscription} from 'rxjs';
import {filter, mergeMap} from 'rxjs/operators';
import {
    PerspectiveCamera,
    Quaternion,
    Vector3
} from 'three';

import {
    createStyles,
    Theme,
    withStyles,
    WithStyles
} from '@material-ui/core/styles';

import brayns, {ifReady, onReady} from '../../common/client';
import {
    CameraChange,
    dispatchCamera,
    dispatchNotification,
    onCameraChange,
    onCameraSettingsChange,
    onViewportChange
} from '../../common/events';
import {compareVectors} from '../../common/math';
import {isCmdKey} from '../../common/utils';

import {EventType, TrackballControls} from './trackball-controls';


const PAN_SPEED_FACTOR = 2;
const ZOOM_SPEED_FACTOR = Math.E;


const styles = (theme: Theme) => createStyles({
    root: {
        display: 'flex',
        position: 'relative',
        flex: 1,
        flexDirection: 'column',
        userSelect: 'none'
    },
    canvas: {
        flex: 1,
        display: 'flex'
    }
});

const style = withStyles(styles);


export class Camera extends PureComponent<Props> {
    private camera?: PerspectiveCamera;
    private controls?: TrackballControls;

    private subs: Subscription[] = [];

    private rootRef: RefObject<HTMLDivElement> = createRef();

    private container: RefObject<HTMLDivElement> = createRef();
    get viewport() {
        const {current} = this.container;
        return current;
    }

    private current = new BehaviorSubject<CameraChange | null>(null);

    inspect = async (evt: MouseEvent) => {
        const viewport = this.viewport;
        if (!viewport) {
            return;
        }

        const {width, height} = viewport.getBoundingClientRect();

        // Only dispatch inspect if the CTRL/CMD key is held down
        if (this.viewport && width > 0 && height > 0 && isCmdKey(evt)) {
            // Do not propagate the event further
            evt.stopPropagation();
            evt.stopImmediatePropagation();

            const box = this.viewport.getBoundingClientRect();
            const coords: InspectParams = [
                (evt.clientX - box.left) / width,
                1 - ((evt.clientY - box.top) / height)
            ];

            try {
                const position = await brayns.request(INSPECT, coords);
                // tslint:disable-next-line: no-console
                console.log(position);
            } catch (err) {
                dispatchNotification(err);
            }
        }
    }

    update = (camera: CameraChange) => {
        if (!this.camera || !this.controls) {
            return;
        }

        const quaternion = new Quaternion(...camera.orientation);
        const position = new Vector3(...camera.position);

        const up = new Vector3(0, 1, 0);
        up.applyQuaternion(quaternion);

        this.camera.position.copy(position);
        this.camera.setRotationFromQuaternion(quaternion);
        this.camera.up.copy(up);

        this.controls.target.fromArray(camera.target ? camera.target : [0, 0, 0]);

        if (isNotEqual(this.current.value, camera)) {
            dispatchCamera(camera);
        }
    }

    resize(width: number, height: number) {
        if (this.camera && this.controls) {
            // Update PerspectiveCamera frustum aspect ratio
            this.camera.aspect = aspectRatio(width, height);
            this.camera.updateProjectionMatrix();
            // Update controls
            this.controls.handleResize();
            this.controls.update();
        }
    }

    componentDidMount() {
        // If there's no container el we can bind the camera to,
        // there's no point in continuing
        const viewport = this.viewport;
        if (!viewport) {
            return;
        }

        // Attach the inspect event as a native evt listener,
        // otherwise we cannot prevent the trackball controls from also trigerring
        // See https://medium.com/@ericclemmons/react-event-preventdefault-78c28c950e46 for more details
        const container = this.rootRef.current;
        if (container) {
            container.addEventListener('mousedown', this.inspect, true);
        }

        // Create camera and controls
        const camera = new PerspectiveCamera(0, 0, 0, 0);
        const controls = new TrackballControls(camera, viewport);
        // Set initial position for the Camera and target for controls (this is necessary,
        // otherwise the change events on controls will never be triggered).
        const position = new Vector3(0, 0, 1);
        const target = new Vector3(0.5, 0.5, 0.5);
        camera.position.copy(position);
        controls.target.copy(target);

        // Ensure that zooming is not weird
        controls.staticMoving = true;

        // Set camera aspect
        const rect = viewport.getBoundingClientRect();
        this.resize(rect.width, rect.height);

        this.camera = camera;
        this.controls = controls;

        const cameraChange = fromEvent(this.controls, EventType.Change)
            .pipe(mergeMap(() => ifReady({
                position: getPosition(camera),
                orientation: getQuaternion(camera),
                target: getTarget(controls)
            })));

        this.subs.push(...[
            // When camera moves we sync with the renderer camera
            // Skip initial change
            cameraChange.subscribe(change => {
                dispatchCamera(change);
            }),
            // Capture camera update events sent from other components
            onCameraChange()
                .pipe(filter(change => isNotEqual(this.current.value, change)))
                .subscribe(camera => {
                    brayns.notify(SET_CAMERA, camera);
                    this.current.next(camera);
                    this.update(camera);
                }),
            onCameraSettingsChange()
                .subscribe(({sensitivity}) => {
                    if (isNumber(sensitivity)) {
                        this.controls!.panSpeed = sensitivity / PAN_SPEED_FACTOR;
                        this.controls!.rotateSpeed = sensitivity;
                        this.controls!.zoomSpeed = ZOOM_SPEED_FACTOR * sensitivity;
                    }
                }),
            // Listen to viewport size updates
            onViewportChange()
                .subscribe(viewport => {
                    const [width, height] = viewport;
                    this.resize(width, height);
                }),
            // Get current state on renderer connect
            onReady().pipe(mergeMap(() => brayns.request(GET_CAMERA)))
                .subscribe(camera => {
                    this.current.next(camera);
                    this.update(camera);
                }),
            brayns.observe(SET_CAMERA)
                .subscribe(camera => {
                    this.current.next(camera);
                    this.update(camera);
                })
        ]);
    }

    componentWillUnmount() {
        // Remove the native event listener for inspect
        const container = this.rootRef.current;
        if (container) {
            container.removeEventListener('mousedown', this.inspect, true);
        }

        // Destroy controls
        if (this.controls) {
            this.controls.dispose();
        }
        for (const sub of this.subs) {
            sub.unsubscribe();
        }
    }

    render() {
        const {classes} = this.props;
        return (
            <div className={classes.root} ref={this.rootRef}>
                <div
                    ref={this.container}
                    className={classes.canvas}
                />
            </div>
        );
    }
}

export default style(Camera);


function aspectRatio(width: number, height: number): number {
    return width / height;
}

function isNotEqual(prev: CameraChange | null, next: CameraChange) {
    return prev !== next
        || !prev
        || !compareVectors(prev.position, next.position)
        || !compareVectors(prev.orientation, next.orientation);
}

function getPosition(camera: PerspectiveCamera) {
    const p = camera.position;
    return p.toArray();
}

function getQuaternion(camera: PerspectiveCamera) {
    const q = new Quaternion();
    q.setFromEuler(camera.rotation);
    return q.toArray();
}

function getTarget(controls: TrackballControls) {
    const target = controls.target;
    return target.toArray();
}


type Props = WithStyles<typeof styles>;
