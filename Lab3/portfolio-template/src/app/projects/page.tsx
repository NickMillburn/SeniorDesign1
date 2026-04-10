export default function Projects() {
  return (
    <section>
      <h1 className="text-3xl font-semibold">Projects</h1>
      <p className="mt-4 text-foreground/70">A collection of things I&apos;ve built.</p>

      <ul className="mt-8 space-y-6">
        <li className="border border-foreground/10 rounded-lg p-5">
          <h2 className="text-lg font-medium">Project One</h2>
          <p className="mt-1 text-sm text-foreground/60">
            Brief description of the project and what it does.
          </p>
        </li>
        <li className="border border-foreground/10 rounded-lg p-5">
          <h2 className="text-lg font-medium">Project Two</h2>
          <p className="mt-1 text-sm text-foreground/60">
            Brief description of the project and what it does.
          </p>
        </li>
      </ul>
    </section>
  );
}
