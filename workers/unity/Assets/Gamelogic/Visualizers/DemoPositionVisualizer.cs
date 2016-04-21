using UnityEngine;
using System.Collections;
using Improbable.Unity.Visualizer;
using Demoteam;
using Improbable.CoreLibrary.CoordinateRemapping;
using Improbable.Unity.Common.Core.Math;
using Improbable.Unity;

public class DemoPositionVisualizer : MonoBehaviour {

    [Require]
    public TransformReader Trans;

	void OnEnable () 
    {
        Trans.PositionUpdated += Trans_PositionUpdated;
        Trans.ForwardUpdated += Trans_ForwardUpdated;
	}

    void Trans_ForwardUpdated(Improbable.Math.Vector3f obj)
    {
        transform.rotation = Quaternion.LookRotation(obj.ToUnityVector());
    }

    void Trans_PositionUpdated(Improbable.Math.Coordinates obj)
    {
        transform.position = obj.RemapGlobalToUnityVector();
    }
}
