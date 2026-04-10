import type { Metadata } from "next";
import { Geist, Geist_Mono } from "next/font/google";
import Link from "next/link";
import "./globals.css";

const geistSans = Geist({ variable: "--font-geist-sans", subsets: ["latin"] });
const geistMono = Geist_Mono({ variable: "--font-geist-mono", subsets: ["latin"] });

export const metadata: Metadata = {
  title: "My Portfolio",
  description: "A personal portfolio website.",
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en">
      <body
        className={`${geistSans.variable} ${geistMono.variable} font-sans antialiased bg-background text-foreground`}
      >
        <header className="sticky top-0 z-10 border-b border-foreground/10 bg-background/80 backdrop-blur">
          <nav className="mx-auto flex max-w-3xl items-center justify-between px-6 py-4">
            <Link href="/" className="text-lg font-semibold">
              Portfolio
            </Link>
            <div className="flex gap-6 text-sm">
              <Link href="/" className="hover:underline">Home</Link>
              <Link href="/projects" className="hover:underline">Projects</Link>
            </div>
          </nav>
        </header>

        <main className="mx-auto max-w-3xl px-6 py-12">
          {children}
        </main>
      </body>
    </html>
  );
}
