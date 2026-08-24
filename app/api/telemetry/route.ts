import { NextResponse } from "next/server";
import { ingest, status, type Packet } from "@/lib/telemetry";

export const dynamic = "force-dynamic";

export async function POST(req: Request) {
  try {
    const body = await req.json();
    const packetList: Packet[] = Array.isArray(body)
      ? body
      : Array.isArray(body.packets)
      ? body.packets
      : [body];

    let lastResult = null;
    for (const pkt of packetList) {
      if (pkt && pkt.nodeId) {
        lastResult = ingest(pkt);
      }
    }
    return NextResponse.json(lastResult ?? status());
  } catch (err: any) {
    return NextResponse.json({ error: err?.message || "Invalid packet format" }, { status: 400 });
  }
}

export async function GET() {
  return NextResponse.json(status());
}
