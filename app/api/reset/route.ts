import { resetSystem } from "@/lib/telemetry";

export const dynamic = "force-dynamic";

export async function POST() {
  try {
    const res = resetSystem();
    return Response.json({ ok: true, message: "System reset successfully", status: res });
  } catch (error) {
    return Response.json({ ok: false, error: "Reset failed" }, { status: 500 });
  }
}

export function GET() {
  const res = resetSystem();
  return Response.json({ ok: true, message: "System reset successfully", status: res });
}
