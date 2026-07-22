# Lab 3 — Personal Portfolio Website

### **Purpose:**
Build a personal portfolio website to present yourself and your work. This lab uses a stripped-down
starter template so each team member can customize it into their own site.

### **Tech Stack**
- [Next.js 15](https://nextjs.org/) (App Router) with Turbopack
- [React 19](https://react.dev/)
- [Tailwind CSS 4](https://tailwindcss.com/)
- [TypeScript](https://www.typescriptlang.org/)

### **Project Structure**
The scaffold lives in `portfolio-template/`:

| Path | Purpose |
| --- | --- |
| `src/app/layout.tsx` | Root layout: site header, navigation, fonts, and shared page shell. |
| `src/app/page.tsx` | Home page — intro/bio (replace the `[Your Name]` placeholder). |
| `src/app/projects/page.tsx` | Projects page listing your work. |
| `src/app/globals.css` | Global styles and Tailwind setup. |

### **Getting Started**
From the `portfolio-template` directory:

```bash
npm install       # install dependencies
npm run dev       # start the dev server at http://localhost:3000
```

Other scripts:

```bash
npm run build     # production build
npm run start     # serve the production build
npm run lint      # run ESLint
```

### **Customizing**
- Update the placeholder name and bio in `src/app/page.tsx`.
- Replace the sample entries in `src/app/projects/page.tsx` with your own projects.
- Adjust the site title/description in the `metadata` export in `src/app/layout.tsx`.
