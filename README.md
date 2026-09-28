# Spongebob 接线可视化网站

这是一个可直接部署的静态网站，用于展示 Spongebob 项目的硬件接线逻辑。

## 文件结构

- `index.html`：页面主体
- `styles.css`：样式文件
- `script.js`：交互脚本

## 本地预览

在当前目录执行：

```bash
python -m http.server 8000
```

然后打开：

```text
http://localhost:8000
```

## 公网部署

### Vercel
1. 上传到 GitHub 仓库
2. 登录 Vercel
3. Import Project
4. 选择该仓库，直接部署即可

### Netlify
1. 上传到 GitHub 仓库
2. 登录 Netlify
3. New site from Git
4. 选择仓库，部署即可

### Cloudflare Pages
1. 上传到 GitHub 仓库
2. 登录 Cloudflare Pages
3. 连接仓库
4. 构建命令留空，输出目录填写 `/`

