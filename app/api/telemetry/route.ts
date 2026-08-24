import { NextResponse } from "next/server";
import { ingest } from "@/lib/telemetry";

export const dynamic = "force-dynamic";

export async function POST(req: Request) {
  try {
    const body = await req.json();
    const packets = Array.isArray(body) ? body : body.packets ?? [body];
    const res = ingest(packets);
    return NextResponse.json(res);
  } catch (err: any) {
    return NextResponse.json({ error: err?.message || "Invalid packet format" }, { status: 400 });
  }
}
