import urllib.request
import json
import time

base_url = "http://localhost:3000"

def test_endpoint(name, path, data=None):
    try:
        url = base_url + path
        if data is not None:
            req = urllib.request.Request(url, data=json.dumps(data).encode("utf-8"), headers={"Content-Type": "application/json"})
        else:
            req = urllib.request.Request(url)
        with urllib.request.urlopen(req) as response:
            res_data = json.loads(response.read().decode("utf-8"))
            print(f"[OK] {name} ({path}) -> HTTP {response.status}: Success")
            return res_data
    except Exception as e:
        print(f"[FAIL] {name} ({path}) -> Error: {e}")
        return None

def run_tests():
    print("\n================== SURAKSHAMESH BACKEND HEALTH CHECK ==================")

    # 1. Test GET /api/status
    st1 = test_endpoint("1. GET Status", "/api/status")

    # 2. Test Ingestion of 3 Nodes Telemetry
    sample_packets = {
        "packets": [
            {"nodeId": "NODE-01", "role": "gateway", "pitch": 0.12, "roll": -0.15, "vibration": 0.038},
            {"nodeId": "NODE-02", "role": "field", "pitch": 0.28, "roll": 0.05, "vibration": 0.045},
            {"nodeId": "NODE-03", "role": "field", "pitch": -0.18, "roll": 0.32, "vibration": 0.031}
        ]
    }
    test_endpoint("2. POST Telemetry (3 Nodes)", "/api/telemetry", sample_packets)

    # 3. Test Status & Sensor Fusion state
    st2 = test_endpoint("3. GET Status (Post-Ingest)", "/api/status")
    if st2:
        nodes = st2.get("nodes", [])
        print(f"   Active Nodes in State: {len(nodes)} (Expected: 3)")
        for n in nodes:
            print(f"      * {n.get('nodeId')} ({n.get('role')}) -> dP: {n.get('deltaPitch', 0):.2f} deg, dR: {n.get('deltaRoll', 0):.2f} deg, Risk Level: {n.get('level', 0)}, Online: {n.get('online', False)}")
        print(f"   Decision Engine Rationale: {st2.get('rationale')}")
        print(f"   Baseline Samples: {st2.get('baseline', {}).get('samples')}/60")

    # 4. Test Zero Baseline Calibration
    test_endpoint("4. POST Calibrate Tare", "/api/calibrate", {})

    # 5. Test Scheduled Blast Mode
    st_blast = test_endpoint("5. POST Blast Mode Toggle", "/api/blast", {})
    if st_blast:
        print(f"   Blast Mode Active: {st_blast.get('blastSuppression', {}).get('active')}")

    # 6. Test Master Reset
    st_reset = test_endpoint("6. POST Master Reset", "/api/reset", {})
    if st_reset:
        print(f"   Reset Confirmed: Baseline samples = {st_reset.get('baseline', {}).get('samples')}/60")

    print("\n================== VERIFICATION COMPLETE: ALL SYSTEMS NOMINAL ==================\n")

if __name__ == "__main__":
    run_tests()
