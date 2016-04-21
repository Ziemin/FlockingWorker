using UnityEngine;
using System.Collections;
using Improbable.Unity.Visualizer;
using Improbable.Unity;

[EngineType(EnginePlatform.FSim)]
public class FaceDirectionOfTravel : MonoBehaviour {

    private Rigidbody Body { get { return _Body == null ? (_Body = GetComponent<Rigidbody>()) : _Body; } }
    private Rigidbody _Body;

    public float RotationSpeed = 1.0f;
    public float SpeedThreshold = 0.12f;

    private float Epsilon = 0.001f;

    private static Vector3 ToXZ(Vector3 v)
    {
        return Vector3.Scale(v, Vector3.right + Vector3.forward);
    }

    private static bool IsZero(Vector3 v, float ep)
    {
        var v2 = Vector3.Scale(v, v);
        return v2.x > ep || v2.y > ep || v2.z > ep;
    }

    private static float Square(float f)
    {
        return f * f;
    }
    	
	void FixedUpdate () {

        if (Body != null)
        {
            var velXZ = ToXZ(Body.velocity);
            var fwdXZ = ToXZ(transform.forward);
            if (!IsZero(velXZ, Epsilon) && !IsZero(fwdXZ, Epsilon)) 
            {
                var twist = Vector3.Cross(fwdXZ.normalized, velXZ.normalized);
                twist = twist.sqrMagnitude > 0.001f ? twist.normalized : Vector3.up;
                Body.angularVelocity += twist * Time.fixedDeltaTime * RotationSpeed;
            }
        }
	}
}
