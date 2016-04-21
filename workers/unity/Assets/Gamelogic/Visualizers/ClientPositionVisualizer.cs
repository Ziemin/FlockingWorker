using UnityEngine;
using System.Collections;
using Improbable.Unity;
using Improbable.Unity.Visualizer;
using Improbable.Corelibrary.Transforms;
using Improbable.Unity.Common.Core.Math;
using Improbable.Corelib.Util;
using Improbable.CoreLibrary.CoordinateRemapping;

public class ClientPositionVisualizer : MonoBehaviour {

    [Require] public TransformStateWriter ImprTransform;

	// Use this for initialization
	void OnEnable () 
    {
        transform.position = ImprTransform.LocalPosition.RemapGlobalToUnityVector();
        transform.rotation = ImprTransform.LocalRotation.ToUnityQuaternion();
	}
	
	// Update is called once per frame
	void Update () 
    {
        ImprTransform.Update.LocalPosition(transform.position.RemapUnityVectorToGlobalVector()).LocalRotation(transform.rotation.ToNativeQuaternion()).FinishAndSend();
	}
}
