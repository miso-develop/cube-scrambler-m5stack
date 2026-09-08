#!/usr/bin/env python3
"""Generate a reusable local CA and an ECDSA server certificate for NanoC6 HTTPS.

The CA private key stays under the local output directory and is never copied to
SPIFFS. The server certificate/private key are copied into the generated Web UI
filesystem so the firmware can load them at runtime. Re-running the script
reuses the same CA by default, avoiding repeated CA installation on clients.
"""

from __future__ import annotations

import argparse
import datetime as dt
import ipaddress
import shutil
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID

DEFAULT_AP_IP = "192.168.4.1"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", default="local/https")
    parser.add_argument("--web-dir", default=".pio/web-ui")
    parser.add_argument("--hostname", default="cube-scrambler.local")
    parser.add_argument(
        "--ip",
        action="append",
        default=[],
        help="Optional IPv4/IPv6 SAN. May be specified more than once.",
    )
    parser.add_argument(
        "--force-ca",
        action="store_true",
        help="Regenerate the local CA. Existing clients must trust the new CA again.",
    )
    return parser.parse_args()


def write_private_key(path: Path, key: ec.EllipticCurvePrivateKey) -> None:
    path.write_bytes(
        key.private_bytes(
            serialization.Encoding.PEM,
            serialization.PrivateFormat.PKCS8,
            serialization.NoEncryption(),
        )
    )


def load_or_create_ca(output_dir: Path, force: bool):
    key_path = output_dir / "root-ca.key.pem"
    cert_path = output_dir / "root-ca.crt"

    if not force and key_path.exists() and cert_path.exists():
        key = serialization.load_pem_private_key(key_path.read_bytes(), password=None)
        cert = x509.load_pem_x509_certificate(cert_path.read_bytes())
        if not isinstance(key, ec.EllipticCurvePrivateKey):
            raise RuntimeError("Existing root CA key is not an EC private key")
        return key, cert, False

    key = ec.generate_private_key(ec.SECP256R1())
    now = dt.datetime.now(dt.timezone.utc)
    subject = issuer = x509.Name(
        [x509.NameAttribute(NameOID.COMMON_NAME, "Cube Scrambler Local Root CA")]
    )
    cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - dt.timedelta(days=1))
        .not_valid_after(now + dt.timedelta(days=3650))
        .add_extension(x509.BasicConstraints(ca=True, path_length=0), critical=True)
        .add_extension(
            x509.KeyUsage(
                digital_signature=True,
                key_encipherment=False,
                key_cert_sign=True,
                key_agreement=False,
                content_commitment=False,
                data_encipherment=False,
                crl_sign=True,
                encipher_only=False,
                decipher_only=False,
            ),
            critical=True,
        )
        .add_extension(
            x509.SubjectKeyIdentifier.from_public_key(key.public_key()), critical=False
        )
        .sign(key, hashes.SHA256())
    )
    write_private_key(key_path, key)
    cert_path.write_bytes(cert.public_bytes(serialization.Encoding.PEM))
    (output_dir / "root-ca.der.crt").write_bytes(
        cert.public_bytes(serialization.Encoding.DER)
    )
    return key, cert, True


def create_server_certificate(
    output_dir: Path,
    ca_key: ec.EllipticCurvePrivateKey,
    ca_cert: x509.Certificate,
    hostname: str,
    ips: list[str],
) -> tuple[Path, Path]:
    key = ec.generate_private_key(ec.SECP256R1())
    now = dt.datetime.now(dt.timezone.utc)
    san_entries: list[x509.GeneralName] = [x509.DNSName(hostname)]
    for value in ips:
        san_entries.append(x509.IPAddress(ipaddress.ip_address(value)))

    cert = (
        x509.CertificateBuilder()
        .subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, hostname)]))
        .issuer_name(ca_cert.subject)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - dt.timedelta(days=1))
        .not_valid_after(now + dt.timedelta(days=397))
        .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=True)
        .add_extension(x509.SubjectAlternativeName(san_entries), critical=False)
        .add_extension(
            x509.KeyUsage(
                digital_signature=True,
                key_encipherment=False,
                key_cert_sign=False,
                key_agreement=True,
                content_commitment=False,
                data_encipherment=False,
                crl_sign=False,
                encipher_only=False,
                decipher_only=False,
            ),
            critical=True,
        )
        .add_extension(
            x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]), critical=False
        )
        .sign(ca_key, hashes.SHA256())
    )

    key_path = output_dir / "server.key.pem"
    cert_path = output_dir / "server.crt"
    write_private_key(key_path, key)
    cert_path.write_bytes(cert.public_bytes(serialization.Encoding.PEM))
    return cert_path, key_path


def main() -> None:
    args = parse_args()
    output_dir = Path(args.output_dir)
    web_dir = Path(args.web_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    web_dir.mkdir(parents=True, exist_ok=True)

    # The firmware uses Arduino's default ESP32 SoftAP address. Include it in
    # every leaf certificate so the same image is usable in AP recovery mode.
    ips = list(dict.fromkeys([DEFAULT_AP_IP, *args.ip]))

    ca_key, ca_cert, ca_created = load_or_create_ca(output_dir, args.force_ca)
    cert_path, key_path = create_server_certificate(
        output_dir, ca_key, ca_cert, args.hostname, ips
    )

    shutil.copyfile(cert_path, web_dir / "tls.crt")
    shutil.copyfile(key_path, web_dir / "tls.key")
    shutil.copyfile(output_dir / "root-ca.crt", web_dir / "ca.crt")

    san = [args.hostname, *ips]
    print(f"Local CA: {'CREATED' if ca_created else 'REUSED'}")
    print(f"Server SAN: {', '.join(san)}")
    print(f"Root CA PEM: {output_dir / 'root-ca.crt'}")
    print(f"Root CA DER: {output_dir / 'root-ca.der.crt'}")
    print(f"SPIFFS certificate: {web_dir / 'tls.crt'}")
    print(f"SPIFFS private key: {web_dir / 'tls.key'}")
    print("IMPORTANT: never commit local/https or the generated tls.key file.")


if __name__ == "__main__":
    main()
