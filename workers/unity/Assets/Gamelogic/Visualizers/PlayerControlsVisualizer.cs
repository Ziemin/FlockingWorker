using UnityEngine;
using System.Collections;
using Improbable.Unity;
using Improbable.Unity.Visualizer;
using Demoteam;

using Transform = UnityEngine.Transform;

[EngineType(EnginePlatform.Client)]
public class PlayerControlsVisualizer : MonoBehaviour {

    [Require] public PlayerWriter Player;
    
    private Vector3 RotationPositionAnchor;
    private Camera Cam;
    private int SteeringMouseButton = 1;

    public Vector3 MovementSpeed = new Vector3(1.0f, 1.0f, 1.0f);

    void OnEnable()
    {
        Cam = GetComponentInChildren<Camera>();
    }
    
    float GetDirectionInput(KeyCode plus, KeyCode minus)
    {
        if (Input.GetKey(plus))
        {
            return 1.0f;
        }
        if (Input.GetKey(minus))
        {
            return -1.0f;
        }
        return 0.0f;
    }

    float GetForwardInput()
    {
        return Input.GetAxis("Vertical") + GetDirectionInput(KeyCode.W, KeyCode.S);
    }

    float GetRightInput()
    {
        return Input.GetAxis("Horizontal") + GetDirectionInput(KeyCode.D, KeyCode.A);
    }

    Vector3 GetMovementVectorMs()
    {
        return Vector3.forward * GetForwardInput() + Vector3.right * GetRightInput();
    }

    static bool Approximately(Vector3 v0, Vector3 v1)
    {
        return Mathf.Approximately(v0.x, v1.x) && Mathf.Approximately(v0.y, v1.y) && Mathf.Approximately(v0.z, v1.z);
    }

    static bool ApproxZero(Vector3 v)
    {
        return Approximately(v, Vector3.zero);
    }

    Vector3 GetMouseScreenPosition()
    {
        return Input.mousePosition + Vector3.forward * Cam.nearClipPlane;
    }

    static Quaternion Shortest(Quaternion q)
    {
        return q.w<0.0f?new Quaternion(-q.x, -q.y, -q.z, -q.w):q;
    }

    float GetSpeedMultiplier()
    {
        var speed = Input.GetAxis("SpeedMultiplier");

        return 1.0f + Mathf.Lerp(0.0f, 9.0f, Mathf.Abs(speed));
    }

    void UpdateMouseLook()
    {
        if (Input.GetMouseButtonDown(SteeringMouseButton))
        {
            RotationPositionAnchor = transform.InverseTransformPoint(Cam.ScreenToWorldPoint(GetMouseScreenPosition()));
        }
        if (Input.GetMouseButton(SteeringMouseButton))
        {
            var curPos = transform.InverseTransformPoint(Cam.ScreenToWorldPoint(GetMouseScreenPosition()));

            if (!ApproxZero(RotationPositionAnchor) && !ApproxZero(curPos))
            {
                var rotationDelta = Quaternion.FromToRotation(RotationPositionAnchor.normalized, curPos.normalized);

                transform.rotation *= rotationDelta;

                var rightXZ = Vector3.Scale(transform.right, Vector3.forward + Vector3.right).normalized;
                transform.rotation = Shortest(Quaternion.FromToRotation(transform.right, rightXZ)) * transform.rotation;
            }

            RotationPositionAnchor = curPos;
        }
    }

    void UpdateJoystickLook()
    {
        transform.rotation = Quaternion.AngleAxis(Input.GetAxis("LookVertical"), transform.right) * transform.rotation;
        transform.rotation = Quaternion.AngleAxis(Input.GetAxis("LookHorizontal"), Vector3.up) * transform.rotation;
    }

    void Update()
    {
        UpdateMouseLook();
        UpdateJoystickLook();

        transform.position += transform.TransformVector(Vector3.Scale(MovementSpeed, GetMovementVectorMs())*GetSpeedMultiplier()) * Time.deltaTime;
    }
}
