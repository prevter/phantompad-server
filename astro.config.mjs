// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import starlightThemeRapide from 'starlight-theme-rapide'

// https://astro.build/config
export default defineConfig({
	integrations: [
		starlight({
			plugins: [starlightThemeRapide()],
			title: 'Phantom Pad',
			social: [{
				icon: 'github',
				label: 'GitHub',
				href: 'https://github.com/prevter/phantompad-server'
			},{
				icon: 'download',
				label: 'Google Play',
				href: 'https://play.google.com/store/apps/details?id=com.prevter.phantompad'
			}],
			sidebar: [
				{
					label: 'Guides',
					items: [
						'guides/installation',
						'guides/usage',
						'guides/uninstalling'
					],
				},
				{
					label: 'Legal',
					autogenerate: { directory: 'legal' },
				}
			],
		}),
	],
});
