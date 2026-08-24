import { NextResponse } from "next/server";
import { status } from "@/lib/telemetry";

export const dynamic = "force-dynamic";

export async function GET() {
  return NextResponse.json(status());
}
