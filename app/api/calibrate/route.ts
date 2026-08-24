import { calibrate, status } from "@/lib/telemetry";

export const dynamic = "force-dynamic";

export async function POST(request: Request) {
  try {
    const body = await request.json().catch(() => ({}));
    const nodeId = (body as any)?.nodeId;
    const res = calibrate(nodeId);
    return Response.json({ ok: true, message: "Baseline calibrated successfully", status: res });
  } catch (error) {
    return Response.json({ ok: false, error: "Calibration failed" }, { status: 500 });
  }
}

export function GET() {
  const res = calibrate();
  return Response.json({ ok: true, message: "Baseline calibrated successfully", status: res });
}
