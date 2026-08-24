import { triggerBlastSuppression } from "@/lib/telemetry";

export const dynamic = "force-dynamic";

export async function POST(request: Request) {
  try {
    const body = await request.json().catch(() => ({}));
    const seconds = Number((body as any)?.seconds ?? 60);
    const res = triggerBlastSuppression(seconds);
    return Response.json({ ok: true, status: res });
  } catch (error) {
    return Response.json({ ok: false, error: "Blast toggle failed" }, { status: 500 });
  }
}

export function GET() {
  const res = triggerBlastSuppression(60);
  return Response.json({ ok: true, status: res });
}
