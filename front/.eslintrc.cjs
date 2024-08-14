module.exports = {
  root: true,
  env: {
    node: true,
  },
  extends: ['plugin:vue/vue3-essential', 'eslint:recommended', '@vue/eslint-config-typescript', 'prettier'],
  plugins: ['prettier'],
  ignores: ['*.json'],
  rules: {
    'prettier/prettier': ['error', { endOfLine: 'lf' }],
    'vue/multi-word-component-names': 'off',
    'vue/require-default-prop': 'off',
    'vue/html-indent': ['error', 2],
    'vue/singleline-html-element-content-newline': 0,
    'vue/component-name-in-template-casing': ['error', 'PascalCase'],
  },
}
