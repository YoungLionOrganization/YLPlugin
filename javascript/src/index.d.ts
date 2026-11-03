/// <reference types="node" />
export type Integer = number | bigint;
export type KeyProvider = (id: string) => Uint8Array | null | undefined;
export interface Options { keys?: KeyProvider; trust?: KeyProvider; cancel?: () => boolean; }
export interface Limits { metadata?: Integer; toc?: Integer; records?: Integer; record?: Integer; total?: Integer; ratio?: Integer; }
export type Permission = string[] | {format: 'bitmask'; registry: string; registry_version: string; value: Integer} | {format: 'bytes'; registry: string; registry_version: string; values: number[]};
export interface Resource { path: string; media: string; variant: string; key_id: string; offset: Integer; stored: Integer; size: Integer; compression: number; encryption: number; extraction: number; alignment: number; sha256: string; }
export interface Input { path: string; file: string; media?: string; variant?: string; key_id?: string; compression?: 0 | 1; extraction?: 0 | 1 | 2; alignment?: number; }
export interface Target { id: string; host: string; api_min: string; api_max: string; entrypoint: string; priority: Integer; fallback: boolean; required_capabilities: string[]; optional_capabilities: string[]; required_permissions: Permission; optional_permissions: Permission; memory: Integer; storage: Integer; }
export interface Entrypoint { id: string; runtime: string; runtime_min: string; runtime_max: string; resource: string; os: string; arch: string; abi: string; }
export interface Host { id: string; api: string; os: string; arch: string; abi?: string; target?: string; runtimes?: Record<string, string>; capabilities?: string[]; permission_formats?: ('strings' | 'bitmask' | 'bytes')[]; permission_registries?: Record<string, string>; grants?: Record<string, Permission>; allow_generic?: boolean; allow_extraction?: boolean; allow_native?: boolean; require_signature?: boolean; memory?: Integer; storage?: Integer; }
export type Status = 'SUPPORTED' | 'UNSUPPORTED' | 'UNKNOWN' | 'NEEDS_RUNTIME' | 'NEEDS_PERMISSION' | 'NEEDS_LICENSE' | 'NEEDS_EXTRACTION' | 'INVALID_SIGNATURE' | 'INVALID_CONTAINER' | 'RESOURCE_LIMIT' | 'UNSUPPORTED_PROFILE';
export interface Check { status: Status; signature_status: string; reasons: string[]; candidates: Record<string, unknown>[]; target?: string; entrypoint?: string; runtime?: string; required_permissions?: Permission; optional_permissions?: Permission; optional_permissions_supported?: boolean; missing_capabilities?: string[]; memory_required?: Integer; storage_required?: Integer; strategy?: 'direct' | 'extracted'; strategy_reason?: string; }
export interface Inspection { container: string; file_size: Integer; signed: boolean; signature_status: string; signing_key_id: string; metadata: string; resources: Resource[]; profile_status: string; profile_error?: string; id?: string; }
export class Package {
  constructor(path: string, limits?: Limits);
  close(): void;
  readonly metadata: string;
  inspect(): Inspection;
  readonly targets: Target[];
  readonly entrypoints: Entrypoint[];
  permissionBytes(target: number, options?: {optional?: boolean}): Buffer;
  read(path: string, options?: Options): Buffer;
  readSlice(path: string, offset: Integer, count: Integer, options?: Options): Buffer;
  validate(options?: Options): void;
  verify(trust: KeyProvider, options?: Options): void;
  check(host: Host, options?: Options): Check;
  extract(destination: string, paths?: string[], options?: Options): void;
}
export function create(output: string, manifest: string, resources: Input[], options?: Options): void;
export function sign(input: string, output: string, keyId: string, privatePem: Uint8Array, options?: Options): void;
export const version: string;
declare const sdk: {Package: typeof Package; create: typeof create; sign: typeof sign; version: string};
export default sdk;
